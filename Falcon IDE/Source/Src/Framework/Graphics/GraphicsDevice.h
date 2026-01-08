#pragma once

class RTVHeap;
class CBVSRVUAVHeap;
class CBufferAllocater;
class DSVHeap;
class DepthStencil;

class GraphicsDevice
{
public:
	
	// <Initialize:初期化処理>　<Arg:ウィンドウハンドル、幅、高>
	bool Init(HWND hWnd, int w, int h);

	// <Pre:描画事前準備処理>
	void PreDraw();
	void PreDraw(ComPtr<ID3D12GraphicsCommandList6>& cmdList) const;

	// <Transition:Viewの切り替え>
	void TransitionResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

	// <Swap:切り替え>
	void ScreenFlip();

	// <Synchronization:コマンドライン同期>
	void WaitForCommandQueue();

	// <Getter:デバイス>
	ID3D12Device8* GetDevice()const { return m_pDevice.Get(); }

	// <Getter:コマンドライン>
	ID3D12GraphicsCommandList6* GetCmdList()const { return m_pCmdList.Get(); }

	// <Getter:コマンドキュー>
	ID3D12CommandQueue* GetCmdQueue()const { return m_pCmdQueue.Get(); }

	// <Getter:定数バッファビュー,シェーダーリソースビュー,アンオーダーアクセスビューのヒープ>
	CBVSRVUAVHeap* GetCBVSRVUAVHeap()const { return m_upCBVSRVUAVHeap.get(); }

	// <Getter:定数バッファアロケーター>
	CBufferAllocater* GetCBufferAllocater()const { return m_upCBufferAllocater.get(); }

	// <Getter:レンダーターゲットビューヒープ>
	const auto GetRTVHeap() const { return m_upRTVHeap.get(); }

	// <Getter:深度,ステンシルビューヒープ>
	DSVHeap* GetDSVHeap()const { return m_upDSVHeap.get(); }

	// <Getter:深度ステンシル>
	const auto& GetDSSRVIndex() const noexcept { return m_upDepthStencil; }
	
	// <Getter:ダブルバッファ>
	const auto GetSwapChainNum()const noexcept { return static_cast<size_t>(SwapBafferNum::Max); }

	// <Getter:バックバッファ>
	const ComPtr<ID3D12Resource>& GetBackBuffer() const noexcept;

	// <Getter:オフスクリーンRTVハンドル>
	const D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenRTVHandle() const noexcept;

	// <Getter:バックバッファインデックス>
	const auto GetCurrentBackBufferIndex() const noexcept { return m_pSwapChain->GetCurrentBackBufferIndex(); }

	// <Getter:指定したインデックスのバックバッファリソースを取得する>
	const ComPtr<ID3D12Resource>& GetBackBufferResource(UINT index) const;

	// <Getter:指定したインデックスのRTVハンドルを取得する>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT index) const;

	// <Getter:オフスクリーンバッファ>
	const auto GetOffsetScreen() const noexcept { return m_pOffscreenBuffer; }

	// <Getter:オフセットスクリーンヒープインデックス>
	const auto GetOffsetScreenIndex() const noexcept { return m_offscreenSRVIndexInGameHeap; }

	// <Setter:VSyncの有効/無効>
	auto SetVsync(bool vsync) noexcept { m_isVsync = vsync; }

private:

	// <Pre:描画事前準備>
	void Prepare();

	// <Create:ファクトリー作成>
	bool CreateFactory();

	// <Create:デバイス作成>
	bool CreateDevice();

	// <Create:コマンドリスト作成>
	bool CreateCommandList();

	// <Create:スワップチェイン作成>　<Arg:ウィンドウハンドル、幅、高>
	bool CreateSwapchain(HWND hWnd, int width, int height);

	// <Create:スワップチェインRTV作成>
	bool CreateSwapchainRTV();

	// <Create:フェンス作成>
	bool CreateFence();

	// <Create:オフスクリーン>
	bool CreateOffsetScreen(const int w, const int h);

	// <Setter:バッファの扱いを変更>　<Arg:指定バッファ、現在の状態、新しい状態>
	void SetResourceBarrier(ID3D12Resource* pResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	// <Debug:デバックレイヤー適応>
	void EnableDebugLayer();

	void SetRenderTarget(ID3D12Resource* pRenderTarget, ID3D12Resource* pDepthStencil, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

	void WaitForGPU();

	enum class GPUTier : size_t
	{
		NVIDIA,
		Amd,
		Intel,
		Arm,
		Qualcomm,
		Kind,
	};

	enum class SwapBafferNum : size_t
	{ Zero, One, Max };

	ComPtr<ID3D12Device8>					m_pDevice;
	ComPtr<IDXGIFactory6>					m_pDxgiFactory;

	ComPtr<ID3D12CommandAllocator>			m_pCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList6>		m_pCmdList;
	ComPtr<ID3D12CommandQueue>				m_pCmdQueue;

	ComPtr<IDXGISwapChain4>						 m_pSwapChain;
	std::array<ComPtr<ID3D12Resource>, 
		static_cast<size_t>(SwapBafferNum::Max)> m_pSwapchainBuffers;

	ComPtr<ID3D12Resource>		m_pOffscreenBuffer{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_offsceenRTVHandle{};
	UINT						m_offscreenSRVIndexInGameHeap{};

	ComPtr<ID3D12Fence>					m_pFence;
	uint64_t							m_fenceVal{ Def::ULongLongZero };
	HANDLE                              m_fenceEvent{ nullptr };

	std::unique_ptr<RTVHeap>			m_upRTVHeap;
	std::unique_ptr<CBVSRVUAVHeap>		m_upCBVSRVUAVHeap;
	std::unique_ptr<CBufferAllocater>	m_upCBufferAllocater;
	std::unique_ptr<DSVHeap>			m_upDSVHeap;
	std::unique_ptr<DepthStencil>		m_upDepthStencil;

	UINT m_rtvDescriptorSize{};
	bool m_isVsync{ false }; // VSyncの有効/無効

	GraphicsDevice()  = default;
	~GraphicsDevice();
public:
	static GraphicsDevice& Instance()
	{
		static GraphicsDevice instance;
		return instance;
	}
};