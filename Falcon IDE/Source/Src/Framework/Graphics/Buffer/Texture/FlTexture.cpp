#include "FlTexture.h"
#include "../../GraphicsDevice.h"

bool FlTexture::Load(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string_view filename, bool renderTarget, bool depthStencil, bool generateMipmap)
{
	Release();
	if (filename.empty()) return false;

	// ファイル名をWideCharへ変換
	std::wstring wFilename = sjis_to_wide(filename.data());

	//------------------------------------
	// 画像読み込み
	//------------------------------------

	// Bind Flags
	UINT bindFlags = D3D12_RESOURCE_FLAG_NONE;
	if (renderTarget) bindFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (depthStencil) bindFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// ※DirectX Texライブラリを使用して画像を読み込む

	DirectX::TexMetadata meta;
	DirectX::ScratchImage image;

	bool bLoaded = false;

	// WIC画像読み込み
	//  WIC_FLAGS_ALL_FRAMES … gifアニメなどの複数フレームを読み込んでくれる
	if (SUCCEEDED(DirectX::LoadFromWICFile(wFilename.c_str(), DirectX::WIC_FLAGS_ALL_FRAMES, &meta, image)))
	{
		bLoaded = true;
	}

	// DDS画像読み込み
	if (bLoaded == false) {
		if (SUCCEEDED(DirectX::LoadFromDDSFile(wFilename.c_str(), DirectX::DDS_FLAGS_NONE, &meta, image)))
		{
			bLoaded = true;
		}
	}

	// TGA画像読み込み
	if (bLoaded == false) {
		if (SUCCEEDED(DirectX::LoadFromTGAFile(wFilename.c_str(), &meta, image)))
		{
			bLoaded = true;
		}
	}

	// HDR画像読み込み
	if (bLoaded == false) {
		if (SUCCEEDED(DirectX::LoadFromHDRFile(wFilename.c_str(), &meta, image)))
		{
			bLoaded = true;
		}
	}

	// 読み込み失敗
	if (bLoaded == false)
	{
		return false;
	}

	// ミップマップ生成
	if (meta.mipLevels == 1 && generateMipmap)
	{
		DirectX::ScratchImage mipChain;
		if (SUCCEEDED(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipChain)))
		{
			image.Release();
			image = std::move(mipChain);
		}
	}

	//------------------------------------
	// テクスチャリソース作成
	//------------------------------------
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment           = 0;
	texDesc.Width               = meta.width;
	texDesc.Height              = static_cast<UINT>(meta.height);
	texDesc.DepthOrArraySize    = static_cast<UINT16>(meta.arraySize);
	texDesc.MipLevels           = static_cast<UINT16>(meta.mipLevels);
	texDesc.Format              = meta.format;
	texDesc.SampleDesc.Count    = 1;
	texDesc.SampleDesc.Quality  = 0;
	texDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags               = static_cast<D3D12_RESOURCE_FLAGS>(bindFlags);

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask      = 1;
	heapProps.VisibleNodeMask       = 1;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_resource)
	);

	if (FAILED(hr))
	{
		return false;
	}

	// アップロード用のリソースを作成
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_resource.Get(), 0, (UINT)image.GetImageCount());

	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	ComPtr<ID3D12Resource> textureUploadHeap;
	hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)
	);

	if (FAILED(hr))
	{
		return false;
	}

	// テクスチャデータをアップロード
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, image.GetImages(), image.GetImageCount(), meta, subresources);

	UpdateSubresources(commandList, m_resource.Get(), textureUploadHeap.Get(), 0, 0, (UINT)subresources.size(), subresources.data());

	// シェーダリソースビューを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvHandle);

	// 画像情報取得
	m_desc = texDesc;
	m_filepath = filename;

	return true;
}

bool FlTexture::Create(ID3D12Resource* pResource)
{
	Release();
	if (pResource == nullptr) return false;

	m_resource = pResource;

	// シェーダリソースビューを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_desc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_desc.MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	GraphicsDevice::Instance().GetDevice()->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvHandle);

	return true;
}

bool FlTexture::Create(ID3D12Device* device, const D3D12_RESOURCE_DESC& desc, const D3D12_SUBRESOURCE_DATA* fillData)
{
	Release();

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask      = 1;
	heapProps.VisibleNodeMask       = 1;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_resource)
	);

	if (FAILED(hr))
	{
		return false;
	}

	if (fillData)
	{
		// アップロード用のリソースを作成
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_resource.Get(), 0, 1);

		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		ComPtr<ID3D12Resource> textureUploadHeap;
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureUploadHeap)
		);

		if (FAILED(hr))
		{
			return false;
		}

		// テクスチャデータをアップロード
		UpdateSubresources(GraphicsDevice::Instance().GetCmdList(), m_resource.Get(), textureUploadHeap.Get(), 0, 0, 1, fillData);
	}

	// シェーダリソースビューを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srvHandle);

	// 画像情報取得
	m_desc = desc;

	return true;
}

bool FlTexture::Create(ID3D12Device* device, int w, int h, DXGI_FORMAT format, UINT arrayCnt, const D3D12_SUBRESOURCE_DATA* fillData)
{
	Release();

	// 作成する2Dテクスチャ設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = w;
	desc.Height = h;
	desc.DepthOrArraySize = (UINT16)arrayCnt;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// 作成
	if (Create(device, desc, fillData) == false) return false;

	return true;
}

bool FlTexture::CreateRenderTarget(ID3D12Device* device, int w, int h, DXGI_FORMAT format, UINT arrayCnt, const D3D12_SUBRESOURCE_DATA* fillData, UINT miscFlags)
{
	Release();

	// 作成する2Dテクスチャ設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = w;
	desc.Height = h;
	desc.DepthOrArraySize = (UINT16)arrayCnt;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | miscFlags);

	// 作成
	if (Create(device, desc, fillData) == false) return false;

	return true;
}

bool FlTexture::CreateDepthStencil(ID3D12Device* device, int w, int h, DXGI_FORMAT format, UINT arrayCnt, const D3D12_SUBRESOURCE_DATA* fillData, UINT miscFlags)
{
	Release();

	// 作成する2Dテクスチャ設定
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = w;
	desc.Height = h;
	desc.DepthOrArraySize = (UINT16)arrayCnt;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | miscFlags);

	// 作成
	if (Create(device, desc, fillData) == false) return false;

	return true;
}

void FlTexture::SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
	m_srvHandle = srvHandle;
}

void FlTexture::Release()
{
	m_resource.Reset();
	m_srvHandle.ptr = 0;
	m_rtvHandle.ptr = 0;
	m_dsvHandle.ptr = 0;
	m_filepath = "";
}
