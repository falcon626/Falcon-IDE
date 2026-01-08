#include "FlBuffer.h"

void FlBuffer::CopyFrom(ID3D12GraphicsCommandList* commandList, const FlBuffer& srcBuffer)
{
	commandList->CopyResource(m_pBuffer.Get(), srcBuffer.GetBuffer());
}

void FlBuffer::WriteData(ID3D12GraphicsCommandList* commandList, const void* pSrcData, UINT size)
{
	void* pMappedData;
	D3D12_RANGE readRange = { 0, 0 };
	HRESULT hr = m_pBuffer->Map(0, &readRange, &pMappedData);
	if (SUCCEEDED(hr))
	{
		memcpy(pMappedData, pSrcData, size);
		m_pBuffer->Unmap(0, nullptr);
	}
}

bool FlBuffer::Create(ID3D12Device8* device, D3D12_HEAP_TYPE heapType, UINT bufferSize, const void* initData)
{
	m_bufSize = bufferSize;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type                  = heapType;
	heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask      = 1;
	heapProps.VisibleNodeMask       = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment           = 0;
	resourceDesc.Width               = bufferSize;
	resourceDesc.Height              = 1;
	resourceDesc.DepthOrArraySize    = 1;
	resourceDesc.MipLevels           = 1;
	resourceDesc.Format              = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count    = 1;
	resourceDesc.SampleDesc.Quality  = 0;
	resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pBuffer)
	);

	if (FAILED(hr))
	{
		return false;
	}

	if (initData)
	{
		void* pMappedData;
		D3D12_RANGE readRange = { 0, 0 };
		hr = m_pBuffer->Map(0, &readRange, &pMappedData);
		if (SUCCEEDED(hr))
		{
			memcpy(pMappedData, initData, bufferSize);
			m_pBuffer->Unmap(0, nullptr);
		}
	}

	return true;
}
