#include "FlEditorAdministrator.h"

bool FlEditorAdministrator::Initialize(const HWND hwnd, const int32_t windowW, const int32_t windowH, const std::weak_ptr<FlFrameRateController>& wpFrame) noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	auto& io{ ImGui::GetIO() };
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      

	io.ConfigWindowsMoveFromTitleBarOnly = true;

	//ImGui::StyleColorsClassic();

	m_upEditorStyle = std::make_unique<FlEditorCascadingStyleSheets>();
	SetupThemes(m_upEditorStyle);

	ImGui_ImplWin32_Init(hwnd);

	auto device{ static_cast<ID3D12Device*>(nullptr) };

	const auto& Graphics{ GraphicsDevice::Instance() };

	auto hr{ Graphics.GetDevice()->QueryInterface(IID_PPV_ARGS(&device)) };
	if (FAILED(hr)) return false;

	auto desc{ D3D12_DESCRIPTOR_HEAP_DESC {} };
	desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 2;
	desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pImGuiSrvHeap));
	if (FAILED(hr)) return false;

	const auto handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_gameViewCPUHandle = m_pImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart();
	m_gameViewCPUHandle.ptr += handleIncrementSize;

	m_gameViewGPUHandle = m_pImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart();
	m_gameViewGPUHandle.ptr += handleIncrementSize;

	auto initInfo{ ImGui_ImplDX12_InitInfo{} };
	initInfo.Device = device;
	initInfo.CommandQueue = Graphics.GetCmdQueue();
	initInfo.NumFramesInFlight = static_cast<int>(Graphics.GetSwapChainNum());
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	initInfo.SrvDescriptorHeap = m_pImGuiSrvHeap.Get();
	initInfo.LegacySingleSrvCpuDescriptor = m_pImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart();
	initInfo.LegacySingleSrvGpuDescriptor = m_pImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart();
	ImGui_ImplDX12_Init(&initInfo);

#include "ja_glyph_ranges.h"
	ImFontConfig config;
	config.MergeMode = true;
	io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("Assets/Font/msgothic.ttc", 13.0f, &config, glyphRangesJapanese);

	m_pImGuiContext = ImGui::GetCurrentContext();

	m_windowW = windowW;
	m_windowH = windowH;

	m_upLogEditor					= std::make_unique<FlLogEditor>();
	m_upFileEditor					= std::make_unique<FlFileEditor>();
	m_upListingEditor				= std::make_unique<FlListingEditor>();
	m_upTerminalEditor				= std::make_unique<FlTerminalEditor>();
	m_upScriptModuleEditor			= std::make_unique<FlScriptModuleEditor>(*m_upTerminalEditor);
	m_upPythonMacroEditor			= std::make_unique<FlPythonMacroEditor>(*m_upTerminalEditor);
	m_upDevCmdEditor				= std::make_unique<FlDeveloperCommandPromptEditor>("FlProject-DX12.sln", *m_upTerminalEditor);
	m_upECSInspectorAndHierarchyEditor = std::make_unique<FlECSInspectorAndHierarchy>();
	m_upEditorCamera				= std::make_unique<FlEditorCamera>(windowW, windowH);

	m_wpFrameRateController = wpFrame;

	m_sceneTex = std::make_unique<Texture>();

	m_sceneTex->Create(&GraphicsDevice::Instance(), m_windowW, m_windowH, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	// ImGuiのヒープ上の、ゲームビュー用に確保した場所にSRVを作成
	device->CreateShaderResourceView(m_sceneTex->GetResource(), &srvDesc, m_gameViewCPUHandle);

	m_upLogEditor->AddLog(Math::Color{ Def::FloatOne, Def::FloatOne,Def::Half,Def::FloatOne },
		"Execution Application");

	return true;
}

void FlEditorAdministrator::Begin() noexcept
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	auto cmdList = GraphicsDevice::Instance().GetCmdList();
	auto backBuffer = GraphicsDevice::Instance().GetBackBuffer().Get();
	auto sceneTexture = m_sceneTex->GetResource();
	auto preCopyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	cmdList->ResourceBarrier(1, &preCopyBarrier);
	cmdList->CopyResource(sceneTexture, backBuffer);

	D3D12_RESOURCE_BARRIER postCopyBarriers[] = {
		// バックバッファは、この後のImGuiの描画処理のために、再びレンダーターゲット状態に戻す。
		CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET),
			// シーンテクスチャは、ImGui::Imageで表示するために、ピクセルシェーダーリソース状態へ遷移させる。
			CD3DX12_RESOURCE_BARRIER::Transition(
				sceneTexture,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	};
	cmdList->ResourceBarrier(_countof(postCopyBarriers), postCopyBarriers);


	// --- ここからImGuiのUI構築準備 ---
	m_dockspaceID = ImGui::GetID("MainDockspace");
	if (m_firstRun && !ImGui::DockBuilderGetNode(m_dockspaceID))
	{
		SetupDockLayout();
		m_firstRun = false;
	}
	auto dockspaceId{ ImGui::GetID("MainDockspace") };
	ImGui::DockSpace(dockspaceId, ImVec2(Def::FloatZero, Def::FloatZero), ImGuiDockNodeFlags_PassthruCentralNode);
}

void FlEditorAdministrator::Update() noexcept
{
	Begin();

	if (ImGui::Begin("FramesPerSecond"))
	{
		if (auto spFrameRateController{ m_wpFrameRateController.lock() })
		{
			ImGui::Text("FPS : %f", spFrameRateController->GetCurrentFPS());
			ImGui::Text("DeltaTime : %f", spFrameRateController->GetDeltaTime());

			if (ImGui::Checkbox("VSync", &spFrameRateController->WorkIsVsync()))
			{
				spFrameRateController->SetIsVsync(spFrameRateController->GetIsVsync());

				m_upLogEditor->AddLog("VSync: %s", spFrameRateController->GetIsVsync() ? "Enabled" : "Disabled");
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Limitless", &spFrameRateController->WorkIsLimitless()))
			{
				spFrameRateController->SetIsLimitless(spFrameRateController->GetIsLimitless());

				m_upLogEditor->AddLog("Limitless: %s", spFrameRateController->GetIsLimitless() ? "Enabled" : "Disabled");
			}
			if (!spFrameRateController->GetIsLimitless())
			{
				ImGui::SliderInt("TargetFrameRate", &m_targetFrameRate, 30, 300);

				spFrameRateController->SetDesiredFPS(static_cast<float>(m_targetFrameRate));
			}
		}
	}
	ImGui::End();

	if (ImGui::Begin("Game Viewport"))
	{
		auto texId{ (ImTextureID)(m_gameViewGPUHandle.ptr) };

		auto windowContentSize     { ImGui::GetContentRegionAvail() };

		auto gameScreenOriginalSize{ ImVec2{static_cast<float>(m_windowW), static_cast<float>(m_windowH)} };

		auto aspectRatio { gameScreenOriginalSize.x / gameScreenOriginalSize.y };
		auto scaledWidth { windowContentSize.x };
		auto scaledHeight{ scaledWidth / aspectRatio };

		if (scaledHeight > windowContentSize.y)
		{
			scaledHeight = windowContentSize.y;
			scaledWidth  = scaledHeight * aspectRatio;
		}

		auto currentCursorPos{ ImGui::GetCursorPos() };
		auto offset{ ImVec2{
				(windowContentSize.x - scaledWidth)  * Def::Half,
				(windowContentSize.y - scaledHeight) * Def::Half
			}
		};

		ImGui::SetCursorPos(ImVec2{ currentCursorPos.x + offset.x, currentCursorPos.y + offset.y });
		ImGui::Image(texId, ImVec2{ scaledWidth, scaledHeight });
	}
	ImGui::End();

	if (ImGui::Begin("Theme"))
	{
		if (ImGui::Button("Light Theme")) {
			m_upEditorStyle->TransitionToTheme("Light", Def::DoubleOne, FlEasing::EaseOutCubic);
		}
		if (ImGui::Button("Red Theme")) {
			m_upEditorStyle->TransitionToTheme("Red", Def::DoubleOne, FlEasing::EaseInOutExpo);
		}
		if (ImGui::Button("ModernDark Theme")) {
			m_upEditorStyle->TransitionToTheme("ModernDark", Def::DoubleOne, FlEasing::EaseInOutQuad);
		}
	}
	ImGui::End();

	if (ImGui::Begin("Option"))
	{
		if (ImGui::Button((m_isStop ? "->" : "||"))) m_isStop = !m_isStop;
		if (ImGui::Button("Explanation"))
			ShellExecuteA(NULL, "open", "https://falcon626.github.io/FlProjectExplanation/", NULL, NULL, SW_SHOWNORMAL);
	}
	ImGui::End();
	
	m_upLogEditor->RenderLog("Log Editor");
	m_upFileEditor->ShowAssetBrowser("Asset Browser");
	m_upTerminalEditor->RenderTerminal("Terminal");

	m_upListingEditor->RenderListingViewer("Assets Guid");


	m_upECSInspectorAndHierarchyEditor->Render("ECSHierarchy", "ECSInspector" ,nullptr, nullptr);

	m_upScriptModuleEditor->Render("ScriptModule");
	m_upPythonMacroEditor->RenderEditor("Plugin");

	m_upDevCmdEditor->Render("DevCmdPrp");

	m_upEditorCamera->RenderCameraParameter("Camera");

	m_upEditorStyle->Update();

	End();
}

void FlEditorAdministrator::EditorCameraUpdate() noexcept
{
	m_upEditorCamera->Update();
}

void FlEditorAdministrator::End() noexcept
{
	auto cmdList = GraphicsDevice::Instance().GetCmdList();
	auto sceneTexture = m_sceneTex->GetResource();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		sceneTexture,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &barrier);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GraphicsDevice::Instance().GetCmdList());
}

void FlEditorAdministrator::SetupDockLayout() const noexcept
{
	ImGui::DockBuilderRemoveNode(m_dockspaceID);
	ImGui::DockBuilderAddNode(m_dockspaceID, ImGuiDockNodeFlags_DockSpace);

	auto main{ ImGuiID{} }, right{ ImGuiID{} }, bottom{ ImGuiID{} }, top{ ImGuiID{} };

	ImGui::DockBuilderSplitNode(m_dockspaceID, ImGuiDir_Left, 0.7f, &main, &right);
	ImGui::DockBuilderSplitNode(main, ImGuiDir_Down, 0.3f, &bottom, nullptr);
	ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.3f, &top, nullptr);

	ImGui::DockBuilderDockWindow("FramesPerSecond", top);
	ImGui::DockBuilderDockWindow("Game Viewport", main);
	ImGui::DockBuilderDockWindow("Log Editor", right);
	ImGui::DockBuilderDockWindow("Asset Browser", bottom);
	ImGui::DockBuilderDockWindow("Terminal", bottom);

	ImGui::DockBuilderFinish(m_dockspaceID);
}

void FlEditorAdministrator::SetupThemes(const std::unique_ptr<FlEditorCascadingStyleSheets>& css)
{
	//FlEditorCascadingStyleSheets::StyleTheme darkTheme;
	//darkTheme.colors = {
	//	{ ImGuiCol_WindowBg, ImVec4(0.12f,0.12f,0.12f,1.0f) },
	//	{ ImGuiCol_Text,     ImVec4(0.95f,0.95f,0.95f,1.0f) },
	//	{ ImGuiCol_TextDisabled,ImVec4(0.05f,0.05f,0.05f,1.0f) },
	//	{ ImGuiCol_Button,   ImVec4(0.20f,0.20f,0.20f,1.0f) },
	//	{ ImGuiCol_ButtonHovered, ImVec4(0.26f,0.59f,0.98f,1.0f) },
	//	// Docking用
	//	{ ImGuiCol_Tab, ImVec4(0.18f,0.18f,0.18f,1.0f) },
	//	{ ImGuiCol_TabActive, ImVec4(0.26f,0.59f,0.98f,1.0f) },
	//	{ ImGuiCol_TabUnfocused, ImVec4(0.18f,0.18f,0.18f,0.8f)},
	//	{ ImGuiCol_TitleBg, ImVec4(0.08f,0.08f,0.08f,1.0f) },
	//	{ ImGuiCol_TitleBgActive, ImVec4(0.12f,0.12f,0.12f,1.0f) },
	//};
	//darkTheme.WindowRounding = 6.0f;
	//darkTheme.FrameRounding = 4.0f;

	FlEditorCascadingStyleSheets::StyleTheme modernDark;
	modernDark.colors = {
		// 基本
		{ ImGuiCol_WindowBg,        ImVec4(0.10f, 0.105f, 0.11f, 1.0f) },
		{ ImGuiCol_ChildBg,         ImVec4(0.10f, 0.105f, 0.11f, 1.0f) },
		{ ImGuiCol_PopupBg,         ImVec4(0.08f, 0.08f, 0.09f, 0.94f) },
		{ ImGuiCol_Border,          ImVec4(0.25f, 0.25f, 0.28f, 0.6f) },
		{ ImGuiCol_Text,            ImVec4(0.92f, 0.92f, 0.94f, 1.0f) },
		{ ImGuiCol_TextDisabled,    ImVec4(0.50f, 0.50f, 0.52f, 1.0f) },

		// フレーム系
		{ ImGuiCol_FrameBg,         ImVec4(0.16f, 0.17f, 0.19f, 1.0f) },
		{ ImGuiCol_FrameBgHovered,  ImVec4(0.20f, 0.50f, 0.65f, 0.78f) },
		{ ImGuiCol_FrameBgActive,   ImVec4(0.20f, 0.60f, 0.80f, 1.0f) },

		// ボタン
		{ ImGuiCol_Button,          ImVec4(0.20f, 0.22f, 0.25f, 1.0f) },
		{ ImGuiCol_ButtonHovered,   ImVec4(0.20f, 0.55f, 0.75f, 1.0f) },
		{ ImGuiCol_ButtonActive,    ImVec4(0.15f, 0.45f, 0.65f, 1.0f) },

		// スクロールバー
		{ ImGuiCol_ScrollbarBg,     ImVec4(0.10f, 0.10f, 0.10f, 0.6f) },
		{ ImGuiCol_ScrollbarGrab,   ImVec4(0.25f, 0.25f, 0.28f, 1.0f) },
		{ ImGuiCol_ScrollbarGrabHovered, ImVec4(0.20f, 0.55f, 0.75f, 1.0f) },
		{ ImGuiCol_ScrollbarGrabActive,  ImVec4(0.15f, 0.45f, 0.65f, 1.0f) },

		// スライダー
		{ ImGuiCol_SliderGrab,      ImVec4(0.20f, 0.55f, 0.75f, 1.0f) },
		{ ImGuiCol_SliderGrabActive,ImVec4(0.15f, 0.45f, 0.65f, 1.0f) },

		// Docking / タブ
		{ ImGuiCol_Tab,             ImVec4(0.14f, 0.14f, 0.15f, 1.0f) },
		{ ImGuiCol_TabHovered,      ImVec4(0.20f, 0.55f, 0.75f, 1.0f) },
		{ ImGuiCol_TabActive,       ImVec4(0.18f, 0.50f, 0.70f, 1.0f) },
		{ ImGuiCol_TabUnfocused,    ImVec4(0.14f, 0.14f, 0.15f, 1.0f) },
		{ ImGuiCol_TabUnfocusedActive, ImVec4(0.18f, 0.50f, 0.70f, 1.0f) },

		// タイトルバー
		{ ImGuiCol_TitleBg,         ImVec4(0.08f, 0.08f, 0.09f, 1.0f) },
		{ ImGuiCol_TitleBgActive,   ImVec4(0.10f, 0.105f, 0.11f, 1.0f) },
		{ ImGuiCol_TitleBgCollapsed,ImVec4(0.05f, 0.05f, 0.05f, 1.0f) },

		// テーブルヘッダ
		{ ImGuiCol_TableHeaderBg,	ImVec4(0.00f,0.00f,0.00f,1.0f)},

		// チェックボックス / ラジオ
		{ ImGuiCol_CheckMark,       ImVec4(0.25f, 0.75f, 0.85f, 1.0f) },

		// セパレーター
		{ ImGuiCol_Separator,       ImVec4(0.25f, 0.25f, 0.28f, 1.0f) },
		{ ImGuiCol_SeparatorHovered,ImVec4(0.20f, 0.55f, 0.75f, 1.0f) },
		{ ImGuiCol_SeparatorActive, ImVec4(0.15f, 0.45f, 0.65f, 1.0f) },

		// ヘッダー
		{ ImGuiCol_Header,          ImVec4(0.20f, 0.22f, 0.25f, 1.0f) },
		{ ImGuiCol_HeaderHovered,   ImVec4(0.20f, 0.55f, 0.75f, 1.0f) },
		{ ImGuiCol_HeaderActive,    ImVec4(0.15f, 0.45f, 0.65f, 1.0f) },
	};

	// 丸みとパディング
	modernDark.WindowRounding = 6.0f;
	modernDark.FrameRounding = 4.0f;
	modernDark.GrabMinSize = 4.0f;
	modernDark.ScrollbarSize = 6.0f;

	FlEditorCascadingStyleSheets::StyleTheme lightTheme = modernDark;

	lightTheme.colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
	lightTheme.colors[ImGuiCol_ChildBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
	lightTheme.colors[ImGuiCol_PopupBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
	lightTheme.colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
	lightTheme.colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	lightTheme.colors[ImGuiCol_Button] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	lightTheme.colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.50f, 0.90f, 1.0f);
	lightTheme.colors[ImGuiCol_FrameBg] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
	lightTheme.colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
	lightTheme.colors[ImGuiCol_FrameBgActive] = ImVec4(0.75f, 0.75f, 0.95f, 1.0f);
	lightTheme.colors[ImGuiCol_Tab] = ImVec4(0.40f, 0.60f, 0.80f, 1.0f);
	lightTheme.colors[ImGuiCol_TabUnfocused] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	lightTheme.colors[ImGuiCol_TableHeaderBg] = ImVec4(0.7f, 0.75f, 0.8f, 1.0f);
	lightTheme.colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.90f, 1.0f);
	lightTheme.colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.50f, 0.80f, 1.0f);
	lightTheme.colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	lightTheme.WindowRounding = 2.0f;
	lightTheme.FrameRounding = 2.0f;

	FlEditorCascadingStyleSheets::StyleTheme redTheme = modernDark;

	redTheme.colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.8f);
	redTheme.colors[ImGuiCol_ChildBg] = ImVec4(0.7f, 0.25f, 0.25f, 1.0f);
	redTheme.colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
	redTheme.colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	redTheme.colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
	redTheme.colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.50f, 0.50f, 1.0f);
	redTheme.colors[ImGuiCol_FrameBg] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
	redTheme.colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.05f, 0.05f, 1.0f);
	redTheme.colors[ImGuiCol_FrameBgActive] = ImVec4(0.95f, 0.45f, 0.45f, 1.0f);
	redTheme.colors[ImGuiCol_Tab] = ImVec4(0.95f, 0.45f, 0.45f, 1.0f);
	redTheme.colors[ImGuiCol_TabUnfocused] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
	redTheme.colors[ImGuiCol_Header] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
	redTheme.colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.05f, 0.05f, 1.0f);
	redTheme.colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.50f, 0.50f, 1.0f);
	
	redTheme.WindowRounding = 5.0f;
	redTheme.FrameRounding = 5.0f;
	redTheme.ScrollbarSize = 1.5f;
	redTheme.TabRounding = 5.0f;

	css->AddTheme("Light", lightTheme);
	css->AddTheme("ModernDark", modernDark);
	css->AddTheme("Red", redTheme);
	   
	css->ApplyTheme("Light");
}

void FlEditorAdministrator::Release() noexcept
{
	m_pImGuiContext = nullptr;

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}