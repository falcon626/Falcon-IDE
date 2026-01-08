#include "Texture.h"

bool Texture::Load(const std::string& filePath)
{
	m_pGraphicsDevice = &GraphicsDevice::Instance();

	auto sizeNeeded{ MultiByteToWideChar(CP_ACP, NULL, filePath.c_str(), -Def::IntOne, NULL, NULL) };
	auto wFilePath { std::wstring(sizeNeeded, Def::WCharZero) };

	MultiByteToWideChar(CP_ACP, NULL, filePath.c_str(), -Def::IntOne, &wFilePath[Def::UIntZero], sizeNeeded);

	auto metadata{ DirectX::TexMetadata {} };
	auto scratchImage{ DirectX::ScratchImage {} };

	const DirectX::Image* pImage{ nullptr };

	auto hr{ DirectX::LoadFromWICFile(wFilePath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage)};

	if (FAILED(hr))
	{
		assert("テクスチャの読み込み失敗");
		return false;
	}

	bool bLoaded = false;

	// WIC画像読み込み
	//  WIC_FLAGS_ALL_FRAMES … gifアニメなどの複数フレームを読み込んでくれる
	if (SUCCEEDED(DirectX::LoadFromWICFile(wFilePath.c_str(), DirectX::WIC_FLAGS_ALL_FRAMES, &metadata, scratchImage)))
	{
		bLoaded = true;
	}

	// DDS画像読み込み
	if (bLoaded == false) 
	{
		if (SUCCEEDED(DirectX::LoadFromDDSFile(wFilePath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage)))
		{
			bLoaded = true;
		}
	}

	// TGA画像読み込み
	if (bLoaded == false) 
	{
		if (SUCCEEDED(DirectX::LoadFromTGAFile(wFilePath.c_str(), &metadata, scratchImage)))
		{
			bLoaded = true;
		}
	}

	// HDR画像読み込み
	if (bLoaded == false) 
	{
		if (SUCCEEDED(DirectX::LoadFromHDRFile(wFilePath.c_str(), &metadata, scratchImage)))
		{
			bLoaded = true;
		}
	}

	// 読み込み失敗
	if (bLoaded == false)
	{
		return false;
	}

	pImage = scratchImage.GetImage(0, 0, 0);

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT64>(metadata.width);
	resDesc.Height = static_cast<UINT>(metadata.height);
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	resDesc.SampleDesc.Count = Def::UIntOne;

	hr = m_pGraphicsDevice->GetDevice()->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pBuffer));

	if (FAILED(hr))
	{
		assert(false && "テクスチャバッファ作成失敗");
		return false;
	}

	hr = m_pBuffer->WriteToSubresource(0, nullptr, pImage->pixels, static_cast<UINT>(pImage->rowPitch), static_cast<UINT>(pImage->slicePitch));

	if (FAILED(hr))
	{
		assert(false && "バッファにテクスチャデータの書き込み失敗");
		return false;
	}

	m_srvNumber = m_pGraphicsDevice->GetCBVSRVUAVHeap()->CreateSRV(m_pBuffer.Get());

	return true;
}

bool Texture::Create(GraphicsDevice* pGraphicsDevice, const UINT width, const UINT height, const DXGI_FORMAT format)
{
	m_pGraphicsDevice = pGraphicsDevice;

	// リソースのヒープ設定（デフォルトヒープ）
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	// リソースの設定
	auto resDesc{ D3D12_RESOURCE_DESC{} };
	resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width            = width;
	resDesc.Height           = height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels        = 1;
	resDesc.Format           = format;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags            = D3D12_RESOURCE_FLAG_NONE; // SRV目的なのでALLOW_RENDER_TARGET等は不要

	// 初期リソースステート：CopyDestにしておく（バックバッファコピー先用）
	auto hr{ m_pGraphicsDevice->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pBuffer)) };

	if (FAILED(hr))
	{
		assert(false && "空テクスチャの作成失敗");
		return false;
	}

	// SRVの作成
	m_srvNumber = m_pGraphicsDevice->GetCBVSRVUAVHeap()->CreateSRV(m_pBuffer.Get());

	return true;
}

bool Texture::Create(GraphicsDevice* pGraphicsDevice, const UINT width, const UINT height, const DXGI_FORMAT format, const D3D12_RESOURCE_STATES states, const D3D12_RESOURCE_FLAGS flags)
{
	m_pGraphicsDevice = pGraphicsDevice;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	auto resDesc{ D3D12_RESOURCE_DESC{} };
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = width;
	resDesc.Height = height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = format;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = flags;

	auto hr{ m_pGraphicsDevice->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		states,
		nullptr,
		IID_PPV_ARGS(&m_pBuffer)) };

	if (FAILED(hr))
	{
		assert(false && "空テクスチャの作成失敗");
		return false;
	}

	return true;
}

bool Texture::CreateRTVTexture(GraphicsDevice* pGraphicsDevice, const UINT width, const UINT height, const DXGI_FORMAT format)
{
	m_pGraphicsDevice = pGraphicsDevice;

	// リソースのヒープ設定（デフォルトヒープ）
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	// リソースの設定
	auto resDesc{ D3D12_RESOURCE_DESC{} };
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = width;
	resDesc.Height = height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = format;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// 初期リソースステート：CopyDestにしておく（バックバッファコピー先用）
	auto hr{ m_pGraphicsDevice->GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pBuffer)) };

	if (FAILED(hr))
	{
		assert(false && "空テクスチャの作成失敗");
		return false;
	}

	// SRVの作成
	m_srvNumber = m_pGraphicsDevice->GetCBVSRVUAVHeap()->CreateSRV(m_pBuffer.Get());

	return true;
}

void Texture::Set(int index)
{
	if (m_cbvCount > -Def::IntOne) index = m_cbvCount;

	m_pGraphicsDevice->GetCmdList()->SetGraphicsRootDescriptorTable
	(index, m_pGraphicsDevice->GetCBVSRVUAVHeap()->GetGPUHandle(m_srvNumber));
}