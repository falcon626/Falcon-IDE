#include "GraphicsDevice.h"

bool GraphicsDevice::Init(HWND hWnd, int w, int h)
{
	m_hWnd = hWnd;

	if (!CreateFactory())
	{
		assert(NULL && "ファクトリー作成失敗");
		return false;
	}

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	if (!CreateDevice())
	{
		assert(NULL && "D3D12デバイス作成失敗");
		return false;
	}

	if (!CreateCommandList())
	{
		assert(NULL && "コマンドリストの作成失敗");
		return false;
	}

	if (!CreateSwapchain(hWnd, w, h))
	{
		assert(NULL && "スワップチェインの作成失敗");
		return false;
	}

	m_upRTVHeap = std::make_unique<RTVHeap>();
	if (!m_upRTVHeap->Create(this, HeapType::RTV, 100))
	{
		assert(NULL && "RTVヒープの作成失敗");
		return false;
	}

	m_upCBVSRVUAVHeap = std::make_unique<CBVSRVUAVHeap>();
	if (!m_upCBVSRVUAVHeap->Create(this, HeapType::CBVSRVUAV, Math::Vector3(1000, 1000, 100)))
	{
		assert(NULL && "CBVSRVUAVヒープの作成失敗");
		return false;
	}

	m_upCBufferAllocater = std::make_unique<CBufferAllocater>();
	m_upCBufferAllocater->Create(this, m_upCBVSRVUAVHeap.get());

	m_upDSVHeap = std::make_unique<DSVHeap>();
	if (!m_upDSVHeap->Create(this, HeapType::DSV, 100))
	{
		assert(NULL && "DSVヒープの作成失敗");
		return false;
	}

	m_upDepthStencil = std::make_unique<DepthStencil>();
	if (!m_upDepthStencil->Create(this, Math::Vector2(static_cast<float>(w), static_cast<float>(h))))
	{
		assert(NULL && "DepthStencilの作成失敗");
		return false;
	}

	if (!CreateSwapchainRTV())
	{
		assert(NULL && "スワップチェインRTVの作成失敗");
		return false;
	}

	if (!CreateFence())
	{
		assert(NULL && "フェンスの作成失敗");
		return false;
	}

	if (!CreateOffsetScreen(w, h))
	{
		assert(NULL && "オフスクリーンの作成失敗");
		return false;
	}

	return true;
}

bool GraphicsDevice::PreDraw()
{
	if (!ResizeSwapchainToClient())
	{
		assert(NULL && "Failed to resize swapchain");
		return false;
	}

	GetCBVSRVUAVHeap()->SetHeap();

	GetCBufferAllocater()->ResetCurrentUseNumber();
	
	Prepare();

	GetCBufferAllocater()->Begin();
	return true;
}

void GraphicsDevice::PreDraw(ComPtr<ID3D12GraphicsCommandList6>& cmdList) const
{
	GetCBVSRVUAVHeap()->SetHeap(cmdList);

	GetCBufferAllocater()->ResetCurrentUseNumber();
}

void GraphicsDevice::TransitionResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	// 同じ状態への不要なバリアは設定しない
	if (stateBefore == stateAfter)
	{
		return;
	}

	auto barrier{ CD3DX12_RESOURCE_BARRIER::Transition(
		pResource,
		stateBefore,
		stateAfter
	) };

	m_pCmdList->ResourceBarrier(Def::UIntOne, &barrier);
}

const ComPtr<ID3D12Resource>& GraphicsDevice::GetBackBuffer() const noexcept
{
	return m_pSwapchainBuffers[GetCurrentBackBufferIndex()];
}

const D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::GetOffscreenRTVHandle() const noexcept
{
	return m_offsceenRTVHandle;
}

const ComPtr<ID3D12Resource>& GraphicsDevice::GetBackBufferResource(UINT index) const
{
	if (index >= static_cast<UINT>(SwapBafferNum::Max))
	{
		throw std::out_of_range("Back buffer index out of range");
	}

	return m_pSwapchainBuffers[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::GetRTVHandle(UINT index) const
{
	return m_upRTVHeap->GetCPUHandle(index);
}

void GraphicsDevice::Prepare()
{
	auto dsvH{ m_upDSVHeap->GetCPUHandle(m_upDepthStencil->GetDSVNumber()) };

	// オフスクリーンバッファをRTV状態へ
	TransitionResource(m_pOffscreenBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// レンダーターゲットをオフスクリーンバッファに設定
	m_pCmdList->OMSetRenderTargets(1, &m_offsceenRTVHandle, false, &dsvH);
	// クリア
	const float clearColor[]{ 0.5f, 0.8f, 1.0f, 1.0f };

	m_pCmdList->ClearRenderTargetView(m_offsceenRTVHandle, clearColor, Def::UIntZero, nullptr);

	// デプスバッファのクリア
	m_upDepthStencil->ClearBuffer();
}

bool GraphicsDevice::ScreenFlip()
{
	auto bbIdx{ m_pSwapChain->GetCurrentBackBufferIndex() };
	SetResourceBarrier(m_pSwapchainBuffers[bbIdx].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	if (FAILED(m_pCmdList->Close()))
	{
		assert(false && "Failed to close command list");
		return false;
	}

	ID3D12CommandList* cmdlists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(Def::UIntOne, cmdlists);

	if (!WaitForCommandQueue())
	{
		return false;
	}

	if (FAILED(m_pCmdAllocator->Reset()))
	{
		assert(false && "Failed to reset command allocator");
		return false;
	}
	if (FAILED(m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr)))
	{
		assert(false && "Failed to reset command list");
		return false;
	}

	UINT presentFlags{};
	BOOL isFullscreen{};
	if (!m_isVsync &&
		(m_swapchainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0 &&
		SUCCEEDED(m_pSwapChain->GetFullscreenState(&isFullscreen, nullptr)) &&
		!isFullscreen)
	{
		presentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	if (FAILED(m_pSwapChain->Present(static_cast<UINT>(m_isVsync), presentFlags)))
	{
		assert(false && "Failed to present swapchain");
		return false;
	}

	return true;
}

bool GraphicsDevice::WaitForCommandQueue()
{
	const auto fenceValue{ ++m_fenceVal };
	if (FAILED(m_pCmdQueue->Signal(m_pFence.Get(), fenceValue)))
	{
		assert(false && "Failed to signal command queue");
		return false;
	}

	if (m_pFence->GetCompletedValue() >= fenceValue)
	{
		return true;
	}

	if (m_fenceEvent == nullptr)
	{
		m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
		if (m_fenceEvent == nullptr)
		{
			assert(false && "Failed to create event for fence completion");
			return false;
		}
	}

	if (FAILED(m_pFence->SetEventOnCompletion(fenceValue, m_fenceEvent)))
	{
		assert(false && "Failed to set fence completion event");
		return false;
	}

	if (WaitForSingleObject(m_fenceEvent, INFINITE) != WAIT_OBJECT_0)
	{
		assert(false && "Failed to wait for fence completion");
		return false;
	}

	return true;
}

bool GraphicsDevice::CreateFactory()
{
	auto flagsDXGI{ Def::UIntZero };
	flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
	auto hr{ CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(m_pDxgiFactory.GetAddressOf())) };

	if (FAILED(hr)) return false;

	BOOL allowTearing{};
	if (SUCCEEDED(m_pDxgiFactory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&allowTearing,
		sizeof(allowTearing))) && allowTearing)
	{
		m_swapchainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	return true;
}

bool GraphicsDevice::CreateDevice()
{
	auto pSelectAdapter{ ComPtr<IDXGIAdapter>{nullptr} };
	auto pAdapters     { std::vector<ComPtr<IDXGIAdapter>>{} };
	auto descs		   { std::vector<DXGI_ADAPTER_DESC>{} };

	// 使用中PCにあるGPUドライバーを検索して、あれば格納する
	for (auto index{ Def::UIntZero }; true; ++index)
	{
		pAdapters.push_back(nullptr);
		HRESULT ret = m_pDxgiFactory->EnumAdapters(index, &pAdapters[index]);

		if (ret == DXGI_ERROR_NOT_FOUND) break;

		descs.push_back({});
		pAdapters[index]->GetDesc(&descs[index]);
	}

	auto gpuTier{ GPUTier::Kind };

	// 優先度の高いGPUドライバーを使用する
	for (auto i{ Def::IntZero }; i < descs.size(); ++i)
	{
		if (std::wstring(descs[i].Description).find(L"NVIDIA") != std::wstring::npos)
		{
			pSelectAdapter = pAdapters[i];
			break;
		}
		else if (std::wstring(descs[i].Description).find(L"Amd") != std::wstring::npos)
		{
			if (gpuTier > GPUTier::Amd)
			{
				pSelectAdapter = pAdapters[i];
				gpuTier = GPUTier::Amd;
			}
		}
		else if (std::wstring(descs[i].Description).find(L"Intel") != std::wstring::npos)
		{
			if (gpuTier > GPUTier::Intel)
			{
				pSelectAdapter = pAdapters[i];
				gpuTier = GPUTier::Intel;
			}
		}
		else if (std::wstring(descs[i].Description).find(L"Arm") != std::wstring::npos)
		{
			if (gpuTier > GPUTier::Arm)
			{
				pSelectAdapter = pAdapters[i];
				gpuTier = GPUTier::Arm;
			}
		}
		else if (std::wstring(descs[i].Description).find(L"Qualcomm") != std::wstring::npos)
		{
			if (gpuTier > GPUTier::Qualcomm)
			{
				pSelectAdapter = pAdapters[i];
				gpuTier = GPUTier::Qualcomm;
			}
		}
	}

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// Direct3Dデバイスの初期化
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(pSelectAdapter.Get(), lv, IID_PPV_ARGS(&m_pDevice)) == S_OK)
		{
			break;		// 生成可能なバージョンが見つかったらループ打ち切り
		}
	}

	if (!m_pDevice)
	{
		return false;
	}

	m_pDevice->SetName(L"DirectX12 Device8");

	return true;
}

bool GraphicsDevice::CreateCommandList()
{
	auto hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCmdAllocator));

	if (FAILED(hr))
	{
		return false;
	}

	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pCmdList));

	if (FAILED(hr))
	{
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// タイムアウトなし
	cmdQueueDesc.NodeMask = 0;										// アダプターを1つしか使わないときは0でいい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// コマンドリストと合わせる

	// キュー生成
	hr = m_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_pCmdQueue));

	if (FAILED(hr))
	{
		return false;
	}

	m_pCmdQueue->SetName(L"Command Queue");

	return true;
}

bool GraphicsDevice::CreateSwapchain(HWND hWnd, int width, int height)
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = width;
	swapchainDesc.Height = height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// フリップ後は速やかに破棄
	swapchainDesc.Flags = m_swapchainFlags;	// ウィンドウとフルスクリーン切り替え可能

	auto hr = m_pDxgiFactory->CreateSwapChainForHwnd(m_pCmdQueue.Get(), hWnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)m_pSwapChain.GetAddressOf());

	if (FAILED(hr))
	{
		return false;
	}

	m_swapchainWidth = static_cast<UINT>(width);
	m_swapchainHeight = static_cast<UINT>(height);

	return true;
}

bool GraphicsDevice::ResizeSwapchainToClient()
{
	if (!m_hWnd || !m_pSwapChain)
	{
		return false;
	}

	RECT clientRect{};
	if (!GetClientRect(m_hWnd, &clientRect))
	{
		return false;
	}

	const auto width = static_cast<UINT>(clientRect.right - clientRect.left);
	const auto height = static_cast<UINT>(clientRect.bottom - clientRect.top);
	if (width == 0 || height == 0 ||
		(width == m_swapchainWidth && height == m_swapchainHeight))
	{
		return true;
	}

	if (!WaitForCommandQueue())
	{
		return false;
	}
	for (auto& buffer : m_pSwapchainBuffers)
	{
		buffer.Reset();
	}

	auto hr = m_pSwapChain->ResizeBuffers(
		0, width, height, DXGI_FORMAT_UNKNOWN, m_swapchainFlags);
	if (FAILED(hr))
	{
		return false;
	}

	for (UINT i = 0; i < static_cast<UINT>(m_pSwapchainBuffers.size()); ++i)
	{
		hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapchainBuffers[i]));
		if (FAILED(hr))
		{
			return false;
		}

		auto name{ L"SwapchainBuffer" + std::to_wstring(i) };
		m_pSwapchainBuffers[i]->SetName(name.c_str());
		m_pDevice->CreateRenderTargetView(
			m_pSwapchainBuffers[i].Get(), nullptr, m_upRTVHeap->GetCPUHandle(i));
	}

	m_swapchainWidth = width;
	m_swapchainHeight = height;

	return true;
}

bool GraphicsDevice::CreateSwapchainRTV()
{
	for (int i = 0; i < (int)m_pSwapchainBuffers.size(); ++i)
	{
		auto hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapchainBuffers[i]));

		if (FAILED(hr))
		{
			return false;
		}

		auto name{ L"SwapchainBuffer" + std::to_wstring(i) };
		m_pSwapchainBuffers[i]->SetName(name.c_str());

		m_upRTVHeap->CreateRTV(m_pSwapchainBuffers[i].Get());
	}

	return true;
}

bool GraphicsDevice::CreateFence()
{
	auto hr = m_pDevice->CreateFence(m_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

	if (FAILED(hr))
	{
		return false;
	}

	m_pFence->SetName(L"Fence");
	return true;
}

bool GraphicsDevice::CreateOffsetScreen(const int w, const int h)
{
	// 初期化関数内
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = w; // 画面サイズに合わせる
	texDesc.Height = h;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 0.5f; // クリアカラー
	clearValue.Color[1] = 0.8f;
	clearValue.Color[2] = 1.0f;
	clearValue.Color[3] = 1.0f;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// オフスクリーン用リソースを作成
	auto hr = m_pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 初期状態はSRVとして使えるようにしておく
		&clearValue,
		IID_PPV_ARGS(&m_pOffscreenBuffer));
	if (FAILED(hr))
	{
		return false;
	}

	// オフスクリーン用のRTVを作成 (RTV用ヒープのどこかに)
	m_upRTVHeap->CreateRTV(m_pOffscreenBuffer.Get(), m_offsceenRTVHandle);

	// オフスクリーン用のSRVを「ゲーム用ヒープ」に作成し、そのインデックスを保存する
	// CBVSRVUAVHeap::CreateSRVがインデックスを返す実装であると仮定
	m_offscreenSRVIndexInGameHeap = m_upCBVSRVUAVHeap->CreateSRV(m_pOffscreenBuffer.Get());

	m_pOffscreenBuffer->SetName(L"OffscreenBuffer");

	return true;
}

void GraphicsDevice::SetResourceBarrier(ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Transition.pResource = pResource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	m_pCmdList->ResourceBarrier(1, &barrier);
}

void GraphicsDevice::EnableDebugLayer()
{
	auto pDebugLayer{ static_cast<ID3D12Debug*>(nullptr) };

	if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugLayer))))
	{
		return;
	}
	pDebugLayer->EnableDebugLayer(); // デバッグレイヤーを有効にする

	auto pDebugLayer1{ static_cast<ID3D12Debug1*>(nullptr) };
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugLayer1))))
	{
		pDebugLayer1->SetEnableGPUBasedValidation(true);
		pDebugLayer1->Release();
	}

	pDebugLayer->Release();
}

void GraphicsDevice::WaitForGPU()
{
	// フェンスが初期化されていない場合は作成
	if (!m_pFence)
	{
		auto hr{ m_pDevice->CreateFence(NULL, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)) };
		if (FAILED(hr))
		{
			assert(false && "Failed To Create Fence");
			return;
		}
		m_fenceVal = Def::ULongLongOne;
	}

	// イベントハンドルを初期化（必要な場合のみ）
	if (m_fenceEvent == nullptr)
	{
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			assert(false && "Failed To Create Fence Event");
			return;
		}
	}

	// コマンドキューにフェンスをシグナル
	{
		auto hr{ m_pCmdQueue->Signal(m_pFence.Get(), m_fenceVal) };
		if (FAILED(hr))
		{
			assert(false && "Failed To Signal Command Queue");
			return;
		}
	}

	// フェンスが完了していない場合、イベントを待機
	if (m_pFence->GetCompletedValue() < m_fenceVal)
	{
		auto hr{ m_pFence->SetEventOnCompletion(m_fenceVal, m_fenceEvent) };
		if (FAILED(hr))
		{
			assert(false && "Failed To Set Fence Completion Event");
			return;
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	++m_fenceVal;
}

void GraphicsDevice::SetRenderTarget(ID3D12Resource* pRenderTarget, ID3D12Resource* pDepthStencil, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	// リソースバリアを設定してレンダーターゲットを描画可能な状態にする
	if (pRenderTarget)
	{
		SetResourceBarrier(pRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	// レンダーターゲットと深度ステンシルビューを設定
	m_pCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, pDepthStencil ? &dsvHandle : nullptr);

	// レンダーターゲットをクリア
	if (pRenderTarget)
	{
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // クリアカラー（黒）
		m_pCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}

	// 深度ステンシルバッファをクリア
	if (pDepthStencil)
	{
		m_pCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}
}


GraphicsDevice::~GraphicsDevice()
{
	WaitForGPU();

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}
