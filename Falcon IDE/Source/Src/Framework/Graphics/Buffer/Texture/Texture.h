#pragma once

class Texture :public Buffer
{
public:

	/// <summary>
	/// テクスチャのロード
	/// </summary>
	/// <param name="pGraphicsDevice">グラフィックスデバイスのポインタ</param>
	/// <param name="filePath">ファイルパス</param>
	/// <returns>ロードが成功したらtrue</returns>
	bool Load(const std::string& filePath);

	/// <summary>
	/// グラフィックスデバイス上に指定された幅、高さ、フォーマットでリソースを作成します。
	/// </summary>
	/// <param name="pGraphicsDevice">リソースを作成するためのGraphicsDeviceへのポインタ。</param>
	/// <param name="width">作成するリソースの幅（ピクセル単位）。</param>
	/// <param name="height">作成するリソースの高さ（ピクセル単位）。</param>
	/// <param name="format">リソースのDXGIフォーマット。</param>
	/// <returns>リソースの作成に成功した場合はtrue、失敗した場合はfalseを返します。</returns>
	bool Create(GraphicsDevice* pGraphicsDevice, const UINT width, const UINT height,const DXGI_FORMAT format);
	
	bool Create(GraphicsDevice* pGraphicsDevice, const UINT width, const UINT height, const DXGI_FORMAT format, const D3D12_RESOURCE_STATES states, const D3D12_RESOURCE_FLAGS flags);

	bool CreateRTVTexture(GraphicsDevice* pGraphicsDevice, const UINT width, const UINT height, const DXGI_FORMAT format);

	/// <summary>
	/// シェーダーリソースとしてセット
	/// </summary>
	/// <param name="index">インデックス</param>
	void Set(int index = Def::IntZero);

	void SetCBVCount(const int index) noexcept { m_cbvCount = index; }

	/// <summary>
	/// SRV番号を取得
	/// </summary>
	/// <returns>SRV番号</returns>
	inline const auto GetSRVNumber() const noexcept { return m_srvNumber; }

private:
	int m_srvNumber{ Def::IntZero };
	int m_cbvCount { -Def::IntOne };
};