#pragma once

#include "Pipeline/Pipeline.h"
#include "RootSignature/RootSignature.h"

struct ShaderVariableInfo
{
	std::string name = "";
	UINT startOffset = Def::UIntZero;
	UINT size        = Def::UIntZero;
};

struct CBufferLayout 
{
	std::string name = "";
	UINT size        = Def::UIntZero;
	std::vector<ShaderVariableInfo> variables{};
};

struct RenderingSetting
{
	std::vector<InputLayout> InputLayouts;
	std::vector<DXGI_FORMAT> Formats;
	CullMode CullMode = CullMode::Back;
	BlendMode BlendMode = BlendMode::Alpha;
	PrimitiveTopologyType PrimitiveTopologyType = PrimitiveTopologyType::Triangle;
	bool IsDepth = true;
	bool IsDepthMask = true;
	int RTVCount = 1;
	bool IsWireFrame = false;

	void Reset()
	{
		InputLayouts.clear();
		Formats.clear();
		CullMode			  = CullMode::Back;
		BlendMode			  = BlendMode::Alpha;
		PrimitiveTopologyType = PrimitiveTopologyType::Triangle;
		IsDepth				  = true;
		IsDepthMask			  = true;
		RTVCount			  = 1;
		IsWireFrame			  = false;
	}
};

class Shader
{
public:
	static auto& Instance()
	{
		static Shader instance;
		return instance;
	}

	const bool Initializer(GraphicsDevice* pGraphicsDevice,
		const uint32_t windowWidth, const uint32_t windowHeight);

	/// <summary>
	/// 作成
	/// </summary>
	void Create();

	/// <summary>
	/// 描画開始
	/// </summary>
	void Begin();
	void Begin(ComPtr<ID3D12GraphicsCommandList6>& cmdList);

	//void DrawTexture(Texture& texture) const;

	/// <summary>
	/// メッシュの描画
	/// </summary>
	/// <param name="mesh">メッシュ</param>
	void DrawMesh(const Mesh& mesh);

	/// <summary>
	/// モデルの描画
	/// </summary>
	/// <param name="modelData">モデルデータ</param>
	void DrawModel(ModelData& modelData, const Math::Matrix& worldMatrix);
	void DrawModel(ModelData& modelData, const Math::Matrix& worldMatrix, ComPtr<ID3D12GraphicsCommandList6>& cmdList);

	/// <summary>
	/// CBVカウント取得
	/// </summary>
	/// <returns>CBVカウント</returns>
	const UINT GetCBVCount() const { return m_cbvCount; }

	/// <summary>
	/// SRV（シェーダーリソースビュー）の数を返します。
	/// </summary>
	/// <returns>m_rangeTypes 内で RangeType::SRV と等しい要素の数。</returns>
	const UINT64 GetSRVCount() const noexcept { return std::count(m_rangeTypes.begin(), m_rangeTypes.end(), RangeType::SRV); }

	/// <summary>
	/// 入力レイアウトのリストを取得します。
	/// </summary>
	/// <returns>入力レイアウト（InputLayout）の std::vector への定数参照。</returns>
	const std::vector<InputLayout>& GetInputLayout() const noexcept { return m_renderingSetting.InputLayouts; }

	void SetBlobs(const ComPtr<ID3DBlob>& vsBlob, const ComPtr<ID3DBlob>& hsBlob,
		const ComPtr<ID3DBlob>& dsBlob, const ComPtr<ID3DBlob>& gsBlob, const ComPtr<ID3DBlob>& psBlob) noexcept
	{
		m_pVSBlob = vsBlob;
		m_pHSBlob = hsBlob;
		m_pDSBlob = dsBlob;
		m_pGSBlob = gsBlob;
		m_pPSBlob = psBlob;
	}

	void SetVS(const ComPtr<ID3DBlob>& vsBlob) noexcept { m_pVSBlob = vsBlob; }
	void SetHS(const ComPtr<ID3DBlob>& hsBlob) noexcept { m_pHSBlob = hsBlob; }
	void SetDS(const ComPtr<ID3DBlob>& dsBlob) noexcept { m_pDSBlob = dsBlob; }
	void SetGS(const ComPtr<ID3DBlob>& gsBlob) noexcept { m_pGSBlob = gsBlob; }
	void SetPS(const ComPtr<ID3DBlob>& psBlob) noexcept { m_pPSBlob = psBlob; }

private:

	/// <summary>
	/// シェーダーのリフレクション
	///	</summary>
	/// <param name="compiledShader">コンパイル済みシェーダー</param>
	/// <param name="renderingSetting">out 描画設定</param>
	void ReflectShaderCBuffers(ComPtr<ID3DBlob> const compiledShader, RenderingSetting& renderingSetting) noexcept;

	void ShaderView(ComPtr<ID3DBlob> const compiledShader, std::vector<RangeType>& rangeTypes) noexcept;

	/// <summary>
	/// マテリアルをセット
	/// </summary>
	/// <param name="material">マテリアル情報</param>
	void SetMaterial(const Material& material);

	GraphicsDevice* m_pDevice = nullptr;

	std::unique_ptr<Pipeline>		m_upPipeline = nullptr;
	std::unique_ptr<RootSignature>	m_upRootSignature = nullptr;

	std::unique_ptr<CBufferData::WorldMatrix>    m_upWorldMatrix;
	std::unique_ptr<CBufferData::BoneTransforms> m_upBoneTransforms;
	std::unique_ptr<CBufferData::IsSkinMesh>	 m_upIsSkinMesh;
	std::unique_ptr<CBufferData::MaterialCBData> m_upMaterialCorlor;

	std::unordered_map<std::string, CBufferLayout> m_cbufferCache;

	ComPtr<ID3DBlob> m_pVSBlob = nullptr;		// 頂点シェーダー
	ComPtr<ID3DBlob> m_pHSBlob = nullptr;		// ハルシェーダー
	ComPtr<ID3DBlob> m_pDSBlob = nullptr;		// ドメインシェーダー
	ComPtr<ID3DBlob> m_pGSBlob = nullptr;		// ジオメトリシェーダー
	ComPtr<ID3DBlob> m_pPSBlob = nullptr;		// ピクセルシェーダー

	uint32_t m_windowWidth  = Def::UIntZero;	// ウィンドウの横幅
	uint32_t m_windowHeight = Def::UIntZero;	// ウィンドウの縦幅

	RenderingSetting m_renderingSetting{};

	std::vector<RangeType> m_rangeTypes{};

	UINT m_cbvCount = 0;
};