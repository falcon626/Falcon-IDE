#include "CBVSRVUAVHeap.h"

int CBVSRVUAVHeap::CreateSRV(ID3D12Resource* pBuffer)
{
	if (m_useCount.y < m_nextRegistNumber)
	{
		assert(false && "確保済みのヒープ領域を超えました。");
		return Def::IntZero;
	}

	auto handle{ m_pHeap->GetCPUDescriptorHandleForHeapStart() };
	handle.ptr += static_cast<UINT64>(m_useCount.x + Def::IntOne) * m_incrementSize + static_cast<UINT64>(m_nextRegistNumber) * m_incrementSize;
	auto srvDesc{ D3D12_SHADER_RESOURCE_VIEW_DESC{} };
	srvDesc.Format = pBuffer->GetDesc().Format;

	if (pBuffer->GetDesc().Format == DXGI_FORMAT_R32_TYPELESS)
	{
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = Def::UIntOne;

	m_pDevice->GetDevice()->CreateShaderResourceView(pBuffer, &srvDesc, handle);

	return m_nextRegistNumber++;
}

int CBVSRVUAVHeap::CreateSRV(ID3D12Resource* pBuffer, const uint32_t idx)
{
	if (m_useCount.y < idx)
	{
		assert(false && "確保済みのヒープ領域を超えました。");
		return Def::IntZero;
	}

	auto handle{ m_pHeap->GetCPUDescriptorHandleForHeapStart() };
	handle.ptr += static_cast<UINT64>(m_useCount.x + Def::IntOne) * m_incrementSize + static_cast<UINT64>(idx) * m_incrementSize;
	auto srvDesc{ D3D12_SHADER_RESOURCE_VIEW_DESC{} };
	srvDesc.Format = pBuffer->GetDesc().Format;

	if (pBuffer->GetDesc().Format == DXGI_FORMAT_R32_TYPELESS)
	{
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = Def::UIntOne;

	m_pDevice->GetDevice()->CreateShaderResourceView(pBuffer, &srvDesc, handle);

	return idx;
}

uint32_t CBVSRVUAVHeap::CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	uint32_t srvIndex = m_nextRegistNumber++;
	auto cpuHandle = GetCPUHandle(srvIndex);

	m_pDevice->GetDevice()->CreateShaderResourceView(resource, &desc, cpuHandle);

	return srvIndex;
}

const D3D12_GPU_DESCRIPTOR_HANDLE CBVSRVUAVHeap::GetGPUHandle(int number)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_pHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(m_incrementSize) * (static_cast<UINT64>(m_useCount.x) + 1);
	handle.ptr += static_cast<UINT64>(m_incrementSize) * number;
	return handle;
}

void CBVSRVUAVHeap::SetHeap()
{
	ID3D12DescriptorHeap* ppHeaps[] = { m_pHeap.Get()};
	m_pDevice->GetCmdList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

void CBVSRVUAVHeap::SetHeap(ComPtr<ID3D12GraphicsCommandList6>& cmdList)
{
	ID3D12DescriptorHeap* ppHeaps[] = { m_pHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}