#pragma once

#include "MeshData/MeshData.h"

enum class InputLayout
{
	POSITION,
	TEXCOORD,
	NORMAL,
	TANGENT,
	COLOR,
	SKININDEX,
	SKINWEIGHT
};

class Texture;

struct MeshFace
{
	UINT Idx[3];
};

struct Material
{
	std::string					Name;						// マテリアルの名前

	std::shared_ptr<Texture>	spBaseColorTex;				// 基本色テクスチャ
	Math::Vector4				BaseColor = { 1,1,1,1 };	// 基本色のスケーリング係数(RGBA)

	std::shared_ptr<Texture>	spMetallicRoughnessTex;		// B:金属製 G:粗さ
	float						Metallic = 0.0f;			// 金属性のスケーリング係数
	float						Roughness = 1.0f;			// 粗さのスケーリング係数

	std::shared_ptr<Texture>	spEmissiveTex;				// 自己発光テクスチャ
	Math::Vector3				Emissive = { 0,0,0 };		// 自己発光のスケーリング係数(RGB)

	std::shared_ptr<Texture>	spNormalTex;				// 法線テクスチャ

	bool						doubleSided{ false };				// カリング
};

class Mesh
{
public:

	/// <summary>
	/// 作成
	/// </summary>
	/// <param name="pGraphicsDevice">グラフィックスデバイスのポインタ</param>
	/// <param name="vertices">頂点情報</param>
	/// <param name=" faces">面情報</param>
	/// <param name=" material">マテリアル情報</param>
	void Create(GraphicsDevice* pGraphicsDevice, const MeshVertex& vertices,
		const std::vector<MeshFace>& faces, const Material& material, const size_t vertexCount);

	/// <summary>
	/// インスタンス描画
	/// </summary>
	/// <param name=" vertexCount">頂点数</param>
	void DrawInstanced(UINT vertexCount)const;

	/// <summary>
	/// インスタンス数を取得
	/// </summary>
	/// <returns>インスタンス数</returns>
	UINT GetInstanceCount()const { return m_instanceCount; }

	/// <summary>
	/// マテリアルの取得
	/// </summary>
	/// <returns>マテリアル情報</returns>
	const Material& GetMaterial()const { return m_material; }

	/// <summary>
	/// 指定された入力レイアウトをセマンティクスレイアウトとして設定します。
	/// </summary>
	/// <param name="layout">設定するInputLayout型の入力レイアウト。</param>
	void SetInputLayout(const std::vector<InputLayout>& layout) noexcept { m_semanticsLayout = layout; }

	/// <summary>
	/// メッシュの名前を設定します。
	/// </summary>
	/// <param name="name">設定するメッシュ名。</param>
	void SetMeshName(const std::string_view& name)noexcept { m_name = name; }

	/// <summary>
	/// メッシュの名前を取得します。
	/// </summary>
	/// <returns>メッシュの名前を表す std::string オブジェクト。</returns>
	const std::string GetMeshName()const noexcept { return m_name; }

private:
	template<typename T>
	void CreateVertexBuffer(const std::vector<T>& data, const UINT stride, 
		const std::wstring_view& name, ID3D12Resource** outBuffer,
		D3D12_VERTEX_BUFFER_VIEW& view, std::vector<D3D12_VERTEX_BUFFER_VIEW>& views, 
		const size_t vertexCount)
	{
		auto elementCount{ UINT64{data.empty() ? vertexCount : data.size()} };

		// <Quick Return:頂点数が不明なら作れない>
		if (elementCount == NULL) return;

		auto bufferSize{ UINT64{stride * elementCount} };
		bufferSize = (bufferSize + 0xff) & ~0xff;

		auto heapProp{ D3D12_HEAP_PROPERTIES{} };
		auto resDesc { D3D12_RESOURCE_DESC{} };

		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

		resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width            = bufferSize;
		resDesc.Height           = Def::UIntOne;
		resDesc.DepthOrArraySize = Def::UShortOne;
		resDesc.MipLevels        = Def::UShortOne;
		resDesc.Format           = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = Def::UIntOne;
		resDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		auto hr{ m_pDevice->GetDevice()->CreateCommittedResource(&heapProp,
			D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(outBuffer)) };

		assert(SUCCEEDED(hr));

		(*outBuffer)->SetName(name.data());

		// 書き込み
		auto mapped{ static_cast<void*>(nullptr) };
		(*outBuffer)->Map(NULL, nullptr, &mapped);

		if (!data.empty()) memcpy(mapped, data.data(), stride * elementCount);
		else memset(mapped, NULL, stride * elementCount);

		(*outBuffer)->Unmap(NULL, nullptr);

		// ビュー
		view.BufferLocation = (*outBuffer)->GetGPUVirtualAddress();
		view.SizeInBytes    = static_cast<UINT>(resDesc.Width);
		view.StrideInBytes  = stride;

		views.push_back(view);
	}

	GraphicsDevice* m_pDevice = nullptr;

	ComPtr<ID3D12Resource>		m_pPosBuffer  = nullptr;
	ComPtr<ID3D12Resource>		m_pUVBuffer   = nullptr;
	ComPtr<ID3D12Resource>		m_pNorBuffer  = nullptr;
	ComPtr<ID3D12Resource>		m_pColBuffer  = nullptr;
	ComPtr<ID3D12Resource>		m_pTanBuffer  = nullptr;
	ComPtr<ID3D12Resource>		m_pSkiIBuffer = nullptr;
	ComPtr<ID3D12Resource>		m_pSkiWBuffer = nullptr;

	ComPtr<ID3D12Resource>		m_pIBuffer = nullptr;

	D3D12_VERTEX_BUFFER_VIEW	m_posView {};
	D3D12_VERTEX_BUFFER_VIEW	m_UVView  {};
	D3D12_VERTEX_BUFFER_VIEW	m_norView {};
	D3D12_VERTEX_BUFFER_VIEW	m_colView {};
	D3D12_VERTEX_BUFFER_VIEW	m_tanView {};
	D3D12_VERTEX_BUFFER_VIEW	m_skiIView{};
	D3D12_VERTEX_BUFFER_VIEW	m_skiWView{};

	std::vector<D3D12_VERTEX_BUFFER_VIEW> m_views;

	D3D12_INDEX_BUFFER_VIEW		m_ibView{};

	std::string m_name{};

	std::vector<InputLayout> m_semanticsLayout;

	UINT m_instanceCount{};
	Material m_material{};

};