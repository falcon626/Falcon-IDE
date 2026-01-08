#pragma once

enum class CullMode
{
	None  = D3D12_CULL_MODE_NONE,
	Front = D3D12_CULL_MODE_FRONT,
	Back  = D3D12_CULL_MODE_BACK,
};

enum class BlendMode
{
	Add,
	Alpha,
	Opaque,
};

enum class PrimitiveTopologyType
{
	Undefined = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
	Point     = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
	Line      = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
	Triangle  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	Patch     = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH,
};

class RootSignature;

class Pipeline
{
public:

	/// <summary>
	/// 描画設定のセット
	/// </summary>
	/// <param name="pGraphicsDevice">グラフィックスデバイスのポインタ</param>
	/// <param name="pRootSignature">ルートシグネチャのポインタ</param>
	/// <param name="inputLayouts">頂点レイアウト情報</param>
	/// <param name="cullMode">カリングモード</param>
	/// <param name="blendMode">ブレンドモード</param>
	/// <param name="topologyType">プリミティブトポロジー</param>
	void SetRenderSettings(GraphicsDevice* pGraphicsDevice, RootSignature* pRootSignature,
		const std::vector<InputLayout>& inputLayouts, CullMode cullMode, BlendMode blendMode,
		PrimitiveTopologyType topologyType, bool isWireFrame = false);

	/// <summary>
	/// 作成
	/// </summary>
	/// <param name="pBlobs">シェーダーデータリスト</param>
	/// <param name="formats">フォーマットリスト</param>
	/// <param name="isDepth">深度テスト</param>
	/// <param name="isDepthMask">深度書き込み</param>
	/// <param name="rtvCount">RTV数</param>
	/// <param name="isWireFrame">ワイヤーフレームかどうか</param>
	void Create(std::vector<ComPtr<ID3DBlob>> pBlobs, const std::vector<DXGI_FORMAT> formats,
    bool isDepth, bool isDepthMask, int rtvCount, bool isWireFrame);

	/// <summary>
	/// パイプラインの取得
	/// </summary>
	/// <returns>パイプライン</returns>
	ID3D12PipelineState* GetPipeline() { return m_pPipelineState.Get(); }

	ID3D12PipelineState* GetPipeline(BlendMode blendMode, CullMode cullMode, PrimitiveTopologyType topologyType,
		bool isDepth, bool isDepthMask, int rtvCount, bool isWireFrame);

	/// <summary>
	/// トポロジータイプの取得
	/// </summary>
	/// <returns>トポロジータイプ</returns>
	PrimitiveTopologyType GetTopologyType() { return m_topologyType; }

private:

	/// <summary>
	/// 入力レイアウトを設定します。
	/// </summary>
	/// <param name="inputElements">設定するD3D12入力要素記述子のベクターへの参照。</param>
	/// <param name="inputLayouts">入力レイアウト情報を含むInputLayout構造体のベクターへの定数参照。</param>
	void SetInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements,
		const std::vector<InputLayout>& inputLayouts);

	/// <summary>
	/// ブレンドモードのセット
	/// </summary>
	/// <param name="blendDesc">レンダーターゲットブレンド情報</param>
	/// <param name="blendMode">ブレンドモード</param>
	void SetBlendMode(D3D12_RENDER_TARGET_BLEND_DESC& blendDesc, BlendMode blendMode);

	std::string GeneratePSOKey(BlendMode blendMode, CullMode cullMode, PrimitiveTopologyType topologyType,
		bool isDepth, bool isDepthMask, int rtvCount, bool isWireFrame) const;

	GraphicsDevice* m_pDevice        = nullptr;
	RootSignature*  m_pRootSignature = nullptr;

	std::vector<InputLayout>	m_inputLayouts;
	CullMode					m_cullMode{};
	BlendMode					m_blendMode{};
	PrimitiveTopologyType		m_topologyType{};

	ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_psoCache;
	std::vector<DXGI_FORMAT> m_rtvFormats;
	
	ComPtr<ID3DBlob> m_pVs;
	ComPtr<ID3DBlob> m_pHs;
	ComPtr<ID3DBlob> m_pDs;
	ComPtr<ID3DBlob> m_pGs;
	ComPtr<ID3DBlob> m_pPs;

	bool m_isWireFrame{ false };
};