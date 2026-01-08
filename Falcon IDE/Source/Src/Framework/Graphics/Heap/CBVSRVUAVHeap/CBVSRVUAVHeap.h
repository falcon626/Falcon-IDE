#pragma once

class CBVSRVUAVHeap :public Heap<Math::Vector3>
{
public:
	CBVSRVUAVHeap() {}
	~CBVSRVUAVHeap() {}

	/// <summary>
	/// SRVの作成
	/// </summary>
	/// <param name="pBuffer">バッファーのポインタ</param>
	/// <returns>ヒープの紐付けられた登録番号</returns>
	int CreateSRV(ID3D12Resource* pBuffer);
	int CreateSRV(ID3D12Resource* pBuffer, const uint32_t idx);
	uint32_t CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

	/// <summary>
	/// SRVのGPU側アドレスを返す
	/// </summary>
	/// <param name="number">登録番号</param>
	/// <returns>SRVのGPU側アドレス</returns>
	const D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int number)override;

	/// <summary>
	/// ヒープをセットする関数
	/// </summary>
	void SetHeap();

	void SetHeap(ComPtr<ID3D12GraphicsCommandList6>& cmdList);

	/// <summary>
	/// ヒープの取得関数
	/// </summary>
	/// <returns>ヒープ</returns>
	ID3D12DescriptorHeap* GetHeap() { return m_pHeap.Get(); }

	/// <summary>
	/// 使用数を取得
	/// </summary>
	/// <returns>使用数</returns>
	const Math::Vector3& GetUseCount() { return m_useCount; }

	/// <summary>
	/// SRV（シェーダーリソースビュー）の使用数を取得します。
	/// </summary>
	/// <returns>現在のSRV使用数（m_nextRegistNumberの値）を返します。</returns>
	const auto GetSRVUseCount() const { return m_nextRegistNumber; }

private:
};