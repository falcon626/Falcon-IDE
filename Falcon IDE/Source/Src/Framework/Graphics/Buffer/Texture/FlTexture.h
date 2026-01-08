#pragma once

//====================================================
//
// テクスチャクラス
//
//====================================================
class FlTexture
{
public:

	//====================================================
	//
	// 取得
	//
	//====================================================

	// 画像のアスペクト比取得
	float GetAspectRatio() const { return static_cast<float>(m_desc.Width) / m_desc.Height; }
	// 画像の幅を取得
	UINT64 GetWidth()  const { return m_desc.Width; }
	// 画像の高さを取得
	UINT   GetHeight() const { return m_desc.Height; }
	// 画像の全情報を取得
	const D3D12_RESOURCE_DESC& GetInfo() const { return m_desc; }
	// ファイルパス取得(Load時のみ)
	const std::string& GetFilepath()     const { return m_filepath; }

	// 画像リソースを取得
	const ID3D12Resource* GetResource() const { return m_resource.Get(); }
	ID3D12Resource* WorkResource()      const { return m_resource.Get(); }

	// シェーダリソースビュー取得
	const D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE WorkSRV()      const { return m_srvHandle; }

	// レンダーターゲットビュー取得
	const D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return m_rtvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE WorkRTV()      const { return m_rtvHandle; }

	// 深度ステンシルビュー取得
	const D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_dsvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE WorkDSV()      const { return m_dsvHandle; }

	//====================================================
	//
	// 画像ファイルからテクスチャ作成
	//
	//====================================================

	// 画像ファイルを読み込む
	// ・filename		… 画像ファイル名
	// ・renderTarget	… レンダーターゲットビューを生成する(レンダーターゲットにする)
	// ・depthStencil	… 深度ステンシルビューを生成する(Zバッファにする)
	// ・generateMipmap	… ミップマップ生成する？
	bool Load(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string_view filename, bool renderTarget = false, bool depthStencil = false, bool generateMipmap = true);

	//====================================================
	//
	// テクスチャ作成
	//
	//====================================================

	// リソースから作成
	// ・pTexture2D	… 画像リソース
	// 戻り値：true … 成功
	bool Create(ID3D12Resource* pResource);

	// desc情報からテクスチャリソースを作成する
	// ・desc		… 作成するテクスチャリソースの情報
	// ・fillData	… バッファに書き込むデータ　nullptrだと書き込みなし
	// 戻り値：true … 成功
	bool Create(ID3D12Device* device, const D3D12_RESOURCE_DESC& desc, const D3D12_SUBRESOURCE_DATA* fillData = nullptr);

	// 通常テクスチャとして作成
	// ※テクスチャリソースを作成し、ShaderResourceViewのみを作成します
	// ・w			… 画像の幅(ピクセル)
	// ・h			… 画像の高さ(ピクセル)
	// ・format		… 画像の形式　DXGI_FORMATを使用
	// ・arrayCnt	… 「テクスチャ配列」を使用する場合、その数。1で通常の1枚テクスチャ
	// ・fillData	… バッファに書き込むデータ　nullptrだと書き込みなし
	// 戻り値：true … 成功
	bool Create(ID3D12Device* device, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT arrayCnt = 1, const D3D12_SUBRESOURCE_DATA* fillData = nullptr);

	// レンダーターゲットテクスチャとして作成
	// ※テクスチャリソースを作成し、ShaderResourceViewのみを作成します
	// ・w            … 画像の幅(ピクセル)
	// ・h            … 画像の高さ(ピクセル)
	// ・format        … 画像の形式　DXGI_FORMATを使用
	// ・arrayCnt    … 「テクスチャ配列」を使用する場合、その数。1で通常の1枚テクスチャ
	// ・fillData    … バッファに書き込むデータ　nullptrだと書き込みなし
	// ・miscFlags    … その他フラグ(キューブマップを作成したい時に必要)
	// 戻り値：true … 成功
	bool CreateRenderTarget(ID3D12Device* device, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT arrayCnt = 1, const D3D12_SUBRESOURCE_DATA* fillData = nullptr, UINT miscFlags = 0);

	// 深度ステンシルテクスチャ(Zバッファ)として作成
	// ・w            … 画像の幅(ピクセル)
	// ・h            … 画像の高さ(ピクセル)
	// ・format        … 画像の形式　DXGI_FORMATを使用
	// ・arrayCnt    … 「テクスチャ配列」を使用する場合、その数。1で通常の1枚テクスチャ
	// ・fillData    … バッファに書き込むデータ　nullptrだと書き込みなし
	// ・miscFlags    … その他フラグ(キューブマップを作成したい時に必要)
	// 戻り値：true … 成功
	bool CreateDepthStencil(ID3D12Device* device, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_R24G8_TYPELESS, UINT arrayCnt = 1, const D3D12_SUBRESOURCE_DATA* fillData = nullptr, UINT miscFlags = 0);

	//====================================================
	//
	// ビューから作成
	//
	//====================================================
	// ShaderResourceViewをセットする
	void SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);

	// 
	FlTexture() {}

	// 
	FlTexture(std::string_view filename)
	{
		// Load(filename); // DirectX 12では、デバイスとコマンドリストが必要なため、コンストラクタでのロードは避ける
	}

	//====================================================
	// 解放
	//====================================================
	void Release();

	// 
	~FlTexture()
	{
		Release();
	}

private:

	// シェーダリソースビュー(読み取り用)
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle = {};
	// レンダーターゲットビュー(書き込み用)
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle = {};
	// 深度ステンシルビュー(Zバッファ用)
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle = {};

	// 画像情報
	D3D12_RESOURCE_DESC m_desc = {};

	// 画像ファイル名(Load時専用)
	std::string m_filepath;

	// リソース
	ComPtr<ID3D12Resource> m_resource;

private:
	// コピー禁止用
	FlTexture(const FlTexture& src) = delete;
	void operator=(const FlTexture& src) = delete;
};
