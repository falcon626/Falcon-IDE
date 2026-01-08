#pragma once

/// <summary>
/// 様々なバッファを作成・操作するクラス
/// Direct3D12のID3D12Resourceを簡単に操作できるようにまとめたものです
/// (例)頂点バッファ、インデックスバッファ、定数バッファなど
/// </summary>
class FlBuffer {
public:

	/// <summary>
	/// バッファインターフェイスを取得
	/// </summary>
	/// <returns>ID3D12Resource*</returns>
	ID3D12Resource* GetBuffer() const { return m_pBuffer.Get(); }

	/// <summary>
	/// バッファインターフェイスのアドレスを取得
	/// </summary>
	/// <returns>ID3D12Resource* const*</returns>
	ID3D12Resource* const* GetAddress() const { return m_pBuffer.GetAddressOf(); }

	/// <summary>
	/// バッファのサイズを取得
	/// </summary>
	/// <returns>UINT</returns>
	UINT GetBufferSize() const { return m_bufSize; }

	/// <summary>
	/// バッファを作成
	/// </summary>
	/// <param name="device">Direct3Dデバイス</param>
	/// <param name="heapType">ヒープの種類 D3D12_HEAP_TYPE定数を指定する</param>
	/// <param name="bufferSize">作成するバッファのサイズ(byte)</param>
	/// <param name="initData">作成時に書き込むデータ nullptrだと何も書き込まない</param>
	/// <returns>bool</returns>
	bool Create(ID3D12Device8* device, D3D12_HEAP_TYPE heapType, UINT bufferSize, const void* initData = nullptr);

	/// <summary>
	/// バッファを解放
	/// </summary>
	void Release()
	{
		m_pBuffer.Reset();
		m_bufSize = 0;
	}

	/// <summary>
	/// バッファへ指定データを書き込み
	/// </summary>
	/// <param name="commandList">ID3D12GraphicsCommandList*</param>
	/// <param name="pSrcData">書き込みたいデータの先頭アドレス</param>
	/// <param name="size">書き込むサイズ(byte)</param>
	void WriteData(ID3D12GraphicsCommandList* commandList, const void* pSrcData, UINT size);

	/// <summary>
	/// GPU上でバッファのコピーを実行する
	/// </summary>
	/// <param name="commandList">ID3D12GraphicsCommandList*</param>
	/// <param name="srcBuffer">コピー元バッファ</param>
	void CopyFrom(ID3D12GraphicsCommandList* commandList, const FlBuffer& srcBuffer);

	FlBuffer() {}
	~FlBuffer() {
		Release();
	}

protected:

	/// <summary>
	/// バッファ本体
	/// </summary>
	ComPtr<ID3D12Resource> m_pBuffer;

	/// <summary>
	/// バッファのサイズ(byte)
	/// </summary>
	UINT m_bufSize = 0;

private:
	FlBuffer(const FlBuffer& src) = delete;
	void operator=(const FlBuffer& src) = delete;
};

/// <summary>
/// 作業データ付き 定数バッファクラス
/// Bufferに、バッファと同じサイズの作業データを持たせたり、
/// 更新した時だけバッファに書き込みを行ったりと、管理を楽にしたクラス
/// </summary>
template<class DataType>
class FlConstantBuffer {
public:

	/// <summary>
	/// 作業領域取得　※変更フラグがONになります
	/// </summary>
	/// <returns>DataType&</returns>
	DataType& Work()
	{
		m_isDirty = true;	// 変更フラグON
		return m_work;
	}

	/// <summary>
	/// 作業領域取得　※読み取り専用　変更フラグは変化しません
	/// </summary>
	/// <returns>const DataType&</returns>
	const DataType& Get() const { return m_work; }

	/// <summary>
	/// バッファアドレス取得
	/// </summary>
	/// <returns>ID3D12Resource* const*</returns>
	ID3D12Resource* const* GetAddress() const { return m_buffer.GetAddress(); }

	/// <summary>
	/// m_workを定数バッファへ書き込む
	/// ※m_isDirtyがtrueの時のみ、バッファに書き込まれる
	/// </summary>
	/// <param name="commandList">ID3D12GraphicsCommandList*</param>
	void Write(ID3D12GraphicsCommandList6* commandList)
	{
		if (m_isDirty)
		{
			m_buffer.WriteData(commandList, &m_work, m_buffer.GetBufferSize());
			m_isDirty = false;
		}
	}

	/// <summary>
	/// DataType型のサイズの定数バッファを作成
	/// </summary>
	/// <param name="device">Direct3Dデバイス</param>
	/// <param name="initData">作成時にバッファに書き込むデータ　nullptrで何も書き込まない</param>
	/// <returns>bool</returns>
	bool Create(ID3D12Device8* device, const DataType* initData = nullptr)
	{
		if (initData)
		{
			m_work = *initData;
		}
		return m_buffer.Create(device, D3D12_HEAP_TYPE_UPLOAD, sizeof(DataType), &m_work);
	}

	/// <summary>
	/// バッファを解放
	/// </summary>
	void Release()
	{
		m_buffer.Release();
		m_isDirty = true;
	}

	~FlConstantBuffer()
	{
		Release();
	}

	FlConstantBuffer() = default;

private:

	/// <summary>
	/// 定数バッファ
	/// </summary>
	FlBuffer m_buffer;

	/// <summary>
	/// 作業用 定数バッファ
	/// この内容がWrite関数で定数バッファ本体に方に書き込まれる
	/// </summary>
	DataType m_work;

	/// <summary>
	/// データ更新フラグ　パフォーメンス向上のため、これがtrueの時だけWrite()でデータ書き込み実行されるようにしています
	/// </summary>
	bool m_isDirty = true;

private:
	FlConstantBuffer(const FlConstantBuffer& src) = delete;
	void operator=(const FlConstantBuffer& src) = delete;
};
