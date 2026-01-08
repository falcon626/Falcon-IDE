#pragma once

class VertexBufferAllocator
{
public:
    VertexBufferAllocator() = default;
    ~VertexBufferAllocator() { Reset(); }

    // 初期化関数：バッファの作成とマップ処理を行う
    void Create(ID3D12Device8* device, size_t bufferSizeInBytes);

    // CPU側の書き込み用ポインタを取得
    void* GetMappedPtr() const { return m_mappedPtr; }

    // GPUのリソース（バッファ）を取得
    ID3D12Resource* GetResource() const { return m_vertexBuffer.Get(); }

    // VertexBufferView を取得
    D3D12_VERTEX_BUFFER_VIEW GetView(UINT strideInBytes) const;

    // バッファサイズ取得
    size_t GetBufferSize() const { return m_bufferSize; }

    void Reset();

private:
    ComPtr<ID3D12Resource> m_vertexBuffer;
    void* m_mappedPtr = nullptr;
    size_t m_bufferSize = 0;
};
