#include "VBufferAllocator.h"

void VertexBufferAllocator::Create(ID3D12Device8* device, size_t bufferSizeInBytes)
{
    m_bufferSize = bufferSizeInBytes;

    auto heapProps { CD3DX12_HEAP_PROPERTIES{D3D12_HEAP_TYPE_UPLOAD} };
    auto bufferDesc{ CD3DX12_RESOURCE_DESC{CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize)} };

    auto hr{ device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)) };

    if (FAILED(hr)) {
		assert(false && "Failed To Create Vertex Buffer Resource"); 
    }

    hr = m_vertexBuffer->Map(Def::UIntZero, nullptr, &m_mappedPtr);
    if (FAILED(hr)) {
		assert(false && "Failed To Map Vertex Buffer"); 
    }
}

D3D12_VERTEX_BUFFER_VIEW VertexBufferAllocator::GetView(UINT strideInBytes) const
{
    auto view{ D3D12_VERTEX_BUFFER_VIEW{} };

    view.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    view.StrideInBytes  = strideInBytes;
    view.SizeInBytes    = static_cast<UINT>(m_bufferSize);

    return view;
}

void VertexBufferAllocator::Reset()
{
    m_vertexBuffer.Reset();
    m_mappedPtr  = nullptr;
    m_bufferSize = Def::UIntZero;
}