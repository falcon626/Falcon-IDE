#include "Application.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	if(FlAssetProtector::DecryptAllToOriginal("CryptedAssets", "Assets"))
	{
		FlFileWatcher fileW;
		fileW.RemDirectory("CryptedAssets");
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
}

Application::~Application()
{
	m_window.Release();
}

void Application::Update()
{
	GraphicsDevice::Instance().PreDraw();

	Shader::Instance().Begin();

	FlEditorAdministrator::Instance().EditorCameraUpdate();

	FlScene::Instance().Update(m_spFrameRateController->GetDeltaTime());

	FlEditorAdministrator::Instance().Update();

	GraphicsDevice::Instance().ScreenFlip();
}

void Application::Execute()
{
	auto& loader{ FlResourceAdministrator::Instance() };
	Shader::Instance().SetBlobs(
		*loader.Get<ComPtr<ID3DBlob>>("Shader/StandardShader/StandardShader_VS.hlsl"),
		nullptr,
		nullptr,
		nullptr,
		*loader.Get<ComPtr<ID3DBlob>>("Shader/StandardShader/StandardShader_PS.hlsl")
	);

	if (!Shader::Instance().Initializer(&GraphicsDevice::Instance(), m_windowW, m_windowH))
	{
		assert(false && L"Shaderの初期化失敗");
		return;
	}

	Shader::Instance().Create();

	FlScene::Instance().Initializer();

	for ( ; ; )
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			if (MessageBoxA(m_window.GetWndHandle(), "本当にゲームを終了しますか？",
				"終了確認", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) End();
		}

		m_spFrameRateController->BeginFrame();

		// <更新関連処理>
		Update();

		auto titleBar{ std::string{"Falcon IDE <Fps = " + std::to_string(m_spFrameRateController->GetCurrentFPS()) + ">"} };
		SetWindowTextA(m_window.GetWndHandle(), titleBar.c_str());

		m_spFrameRateController->EndFrame();

		if (!m_window.ProcessMessage()) End();
		if (m_isEnd) break;
	}

	FlScene::Instance().PostProcess();

	if (FlAssetProtector::EncryptAllInDirectory("Assets", "CryptedAssets"))
	{
		FlFileWatcher fileW;
		fileW.RemDirectory("Assets");
	}

	// <Release>
	Release();	
}

void Application::Release()
{
	// <Release>
	m_window.Release();
	// </Release>
}