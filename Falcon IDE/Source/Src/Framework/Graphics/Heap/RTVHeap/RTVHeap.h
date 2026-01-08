#pragma once

class RTVHeap :public Heap<int>
{
public:
	RTVHeap() {}
	~RTVHeap() {}

	/// <summary>
	/// RTVの作成
	/// </summary>
	/// <param name="pBuffer">バッファーのポインタ</param>
	/// <returns>ヒープの紐付けられた登録番号</returns>
	int CreateRTV(ID3D12Resource* pBuffer);
	int CreateRTV(ID3D12Resource* pBuffer, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

	const auto& GetBuffer() const noexcept { return m_pBuffer; }
	auto& WorkBuffer() noexcept { return m_pBuffer; }

private:
	ComPtr<ID3D12Resource> m_pBuffer;
};