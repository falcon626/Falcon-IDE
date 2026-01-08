#include "RTVHeap.h"

int RTVHeap::CreateRTV(ID3D12Resource* pBuffer)
{
	pBuffer->SetName(L"RTV Heap");

	if (m_useCount < m_nextRegistNumber)
	{
		assert(false && "確保済みのヒープ領域を超えました。");
		return NULL;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += (UINT64)m_nextRegistNumber * m_incrementSize;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_pDevice->GetDevice()->CreateRenderTargetView(pBuffer, &rtvDesc, handle);
	m_pBuffer = pBuffer;
	return m_nextRegistNumber++;
}

int RTVHeap::CreateRTV(ID3D12Resource* pBuffer, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	if (m_useCount < m_nextRegistNumber)
	{
		assert(false && "確保済みのヒープ領域を超えました。");
		return NULL;
	}

	cpuHandle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += (UINT64)m_nextRegistNumber * m_incrementSize;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_pDevice->GetDevice()->CreateRenderTargetView(pBuffer, &rtvDesc, cpuHandle);
	m_pBuffer = pBuffer;
	return m_nextRegistNumber++;
}
