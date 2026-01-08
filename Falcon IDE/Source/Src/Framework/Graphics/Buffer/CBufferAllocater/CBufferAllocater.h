#pragma once

class CBufferAllocater :public Buffer
{
public:

	/// <summary>
	/// 作成
	/// </summary>
	/// <param name="pGraphicsDevice">グラフィックスデバイスのポインタ</param>
	/// <param name="pHeap">CBVSRVUAVHeapのポインタ</param>
	void Create(GraphicsDevice* pGraphicsDevice, CBVSRVUAVHeap* pHeap);

	/// <summary>
	/// 使用しているバッファの番号を初期化
	/// </summary>
	void ResetCurrentUseNumber();

	void Begin()
	{
		m_currentUseNumber = 0; // フレームの先頭でリセット
	}

	/// <summary>
	/// 定数バッファーにデータのバインドを行う
	/// </summary>
	/// <param name="descIndex">レジスタ番号</param>
	/// <param name="data">バインドデータ</param>
	template<typename T>
	void BindAndAttachData(int descIndex, const T& data);

	template<typename _T>
	void BindAndAttachData(int descIndex, const _T& data, ComPtr<ID3D12GraphicsCommandList6>& cmdList);

private:
	CBVSRVUAVHeap* m_pHeap = nullptr;
	struct { char buf[256]; }*m_pMappedBuffer = nullptr;
	int m_currentUseNumber = 0;
};

template<typename T>
inline void CBufferAllocater::BindAndAttachData(int descIndex, const T& data)
{
	if (!m_pHeap)return;

	// dataサイズを256アライメントして計算
	int sizeAligned = (sizeof(T) + 0xff) & ~0xff;

	// 256byteをいくつ使用するかアライメントした結果を256で割って計算
	int useValue = sizeAligned / 0x100;
	if (useValue == 0) useValue = 1; // データサイズが256バイト未満でも、最低1つは消費する

	// 現在使い終わっている番号と今から使う容量がヒープの容量を超えている場合はリターン
	if (m_currentUseNumber + useValue > static_cast<int>(m_pHeap->GetUseCount().x))
	{
		assert(false && "使用できるヒープ容量を超えました");
		return;
	}

	auto top{ m_currentUseNumber };

	memcpy(reinterpret_cast<BYTE*>(m_pMappedBuffer) + static_cast<size_t>(top) * 0x100, &data, sizeof(T));

	// ビューを作って値をシェーダーにアタッチ
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.BufferLocation = m_pBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(top) * 0x100;
	cbDesc.SizeInBytes = sizeAligned;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pHeap->GetHeap()->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += static_cast<UINT64>(m_pGraphicsDevice->GetDevice()->GetDescriptorHandleIncrementSize
	(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *top); // top を使用

	m_pGraphicsDevice->GetDevice()->CreateConstantBufferView(&cbDesc, cpuHandle);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_pHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += static_cast<UINT64>(m_pGraphicsDevice->GetDevice()->GetDescriptorHandleIncrementSize
	(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *top); // top を使用

	m_pGraphicsDevice->GetCmdList()->SetGraphicsRootDescriptorTable(descIndex, gpuHandle);

	m_currentUseNumber += useValue;
}

template<typename _T>
inline void CBufferAllocater::BindAndAttachData(int descIndex, const _T& data, ComPtr<ID3D12GraphicsCommandList6>& cmdList)
{
	if (!m_pHeap)return;

	// dataサイズを256アライメントして計算
	int sizeAligned = (sizeof(_T) + 0xff) & ~0xff;

	// 256byteをいくつ使用するかアライメントした結果を256で割って計算
	int useValue = sizeAligned / 0x100;

	// 現在使い終わっている番号と今から使う容量がヒープの容量を超えている場合はリターン
	if (m_currentUseNumber + useValue > static_cast<int>(m_pHeap->GetUseCount().x))
	{
		assert(false && "使用できるヒープ容量を超えました");
		return;
	}

	auto top{ m_currentUseNumber };

	// 先頭アドレスに使う分のポインタを足してmemcpy
	memcpy(m_pMappedBuffer + top, &data, sizeof(_T));

	// ビューを作って値をシェーダーにアタッチ
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	cbDesc.BufferLocation = m_pBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(top) * 0x100;
	cbDesc.SizeInBytes = sizeAligned;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pHeap->GetHeap()->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += static_cast<UINT64>(m_pGraphicsDevice->GetDevice()->GetDescriptorHandleIncrementSize
	(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *m_currentUseNumber);

	m_pGraphicsDevice->GetDevice()->CreateConstantBufferView(&cbDesc, cpuHandle);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_pHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += static_cast<UINT64>(m_pGraphicsDevice->GetDevice()->GetDescriptorHandleIncrementSize
	(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *m_currentUseNumber);

	cmdList->SetGraphicsRootDescriptorTable(descIndex, gpuHandle);

	m_currentUseNumber += useValue;
}