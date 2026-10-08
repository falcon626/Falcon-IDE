#include "Application.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	if (!FlAssetProtector::RestoreAssetsIfMissing("CryptedAssets", "Assets"))
	{
		MessageBoxW(nullptr, L"Unable to restore assets. Existing files have been kept.", L"Falcon IDE", MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
	// メモリリークを知らせる
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// COM初期化
	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
	{
		CoUninitialize();

		return NULL;
	}

	// mbstowcs_s関数で日本語対応にするために呼ぶ
	setlocale(LC_ALL, "japanese");

	// DPI設定
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

//#if _DEBUG
//	if (!GetModuleHandle(L"WinPixGpuCapturer.dll")) {
//		PIXLoadLatestWinPixGpuCapturerLibrary();
//	}
//#endif // _DEBUG

	//===================================================================
	// 実行
	//===================================================================
	Application::Instance().Execute();

	// COM解放
	CoUninitialize();

	return NULL;
}

Application::Application()
{
	auto path{ std::string{"Assets/Data/WindowParameters/windowParametersFHD.dat"} };
	//auto path{ std::string{"Assets/Data/WindowParameters/windowParametersHD.dat"} };

	auto parameter{ std::make_shared<std::vector<int>>() };
	//parameter = FlBinaryManager::Instance().LoadData<int>(path);

	uint32_t element{ NULL };

	//m_windowW = (*parameter)[element++];
	//m_windowH = (*parameter)[element++];

	m_windowW = 1920;
	m_windowH = 1080;

	if (!m_window.Create(m_windowW, m_windowH, "FrameworkDX12", "Window"))
	{
		assert(false && "ウィンドウ作成失敗。");
		return;
	}

	if (!FlInput::Instance().Initialize(m_window.GetWndHandle()))
	{
		assert(false && "Input initialization failed.");
		return;
	}

	if (!GraphicsDevice::Instance().Init(m_window.GetWndHandle(), m_windowW, m_windowH))
	{
		assert(false && "グラフィックスデバイス初期化失敗。");
		return;
	}

	m_spFrameRateController = std::make_shared<FlFrameRateController>(1000.0f, 10.0f);
	m_spFrameRateController->SetWindowHandle(m_window.GetWndHandle());

	if (!FlEditorAdministrator::Instance().Initialize(m_window.GetWndHandle(), m_windowW, m_windowH, m_spFrameRateController))
	{
		assert(false && L"FlEditorAdministratorの初期化失敗");
		return;
	}

	m_isReady = true;
}

Application::~Application()
{
	Release();
}

void Application::Update()
{
	if (!GraphicsDevice::Instance().PreDraw())
	{
		End();
		return;
	}

	Shader::Instance().Begin();

	FlEditorAdministrator::Instance().EditorCameraUpdate();

	FlScene::Instance().Update(m_spFrameRateController->GetDeltaTime());

	FlEditorAdministrator::Instance().Update();

	if (!GraphicsDevice::Instance().ScreenFlip())
	{
		End();
		return;
	}
}

void Application::Execute()
{
	if (!m_isReady)
	{
		Release();
		return;
	}

	auto& loader{ FlResourceAdministrator::Instance() };
	m_resourcesReady = true;
	const auto vertexShader = loader.Get<ComPtr<ID3DBlob>>("Shader/StandardShader/StandardShader_VS.hlsl");
	const auto pixelShader = loader.Get<ComPtr<ID3DBlob>>("Shader/StandardShader/StandardShader_PS.hlsl");
	if (!vertexShader || !pixelShader || !vertexShader->Get() || !pixelShader->Get())
	{
		MessageBoxW(m_window.GetWndHandle(), L"Required shaders could not be loaded. Assets have been kept.", L"Falcon IDE", MB_OK | MB_ICONERROR);
		loader.GetMetaFileManager()->StopMonitoring();
		Release();
		return;
	}
	Shader::Instance().SetBlobs(*vertexShader, nullptr, nullptr, nullptr, *pixelShader);

	if (!Shader::Instance().Initializer(&GraphicsDevice::Instance(), m_windowW, m_windowH))
	{
		assert(false && L"Shaderの初期化失敗");
		Release();
		return;
	}

	Shader::Instance().Create();

	FlScene::Instance().Initializer();

	for ( ; ; )
	{
		if (!m_window.ProcessMessage()) End();
		if (m_isEnd) break;

		auto& input{ FlInput::Instance() };
		if (!input.BeginFrame())
		{
			assert(false && "Input frame update failed.");
			End();
			break;
		}

		if (input.IsKeyPressed(FlKey::Escape))
		{
			if (MessageBoxA(m_window.GetWndHandle(), "本当にゲームを終了しますか？",
				"終了確認", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) End();
		}
		if (m_isEnd)
		{
			input.EndFrame();
			break;
		}

		m_spFrameRateController->BeginFrame();

		// <更新関連処理>
		Update();

		auto titleBar{ std::string{"Falcon IDE <Fps = " + std::to_string(m_spFrameRateController->GetCurrentFPS()) + ">"} };
		SetWindowTextA(m_window.GetWndHandle(), titleBar.c_str());

		input.EndFrame();
		m_spFrameRateController->EndFrame();

		if (m_isEnd) break;
	}

	loader.GetMetaFileManager()->StopMonitoring();
	bool saved = FlScene::Instance().PostProcess();
	while (!saved && MessageBoxW(nullptr,
		L"Scene save failed. Retry to save, or Cancel to exit with the previous files preserved. Unsaved edits will be lost.",
		L"Falcon IDE", MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
	{
		saved = FlScene::Instance().PostProcess();
	}
	if (saved && !FlAssetProtector::EncryptAllInDirectory("Assets", "CryptedAssets"))
		MessageBoxW(nullptr, L"Asset archive failed. The working copy has been kept and will be used on the next launch.", L"Falcon IDE", MB_OK | MB_ICONERROR);

	// <Release>
	Release();	
}

void Application::Release()
{
	// <Release>
	m_isReady = false;
	if (m_resourcesReady)
	{
		FlResourceAdministrator::Instance().GetMetaFileManager()->StopMonitoring();
		m_resourcesReady = false;
	}
	FlInput::Instance().Shutdown();
	m_window.Release();
	// </Release>
}
