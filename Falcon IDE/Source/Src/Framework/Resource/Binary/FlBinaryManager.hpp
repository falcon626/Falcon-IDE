#pragma once

#include "FlBinaryAccessor.hpp"

/// <summary> =Flyweight= </summary>
class FlBinaryManager
{
public:

	/// <summary>
	/// 指定されたファイルからデータを読み込み、T型のstd::vector配列 の共有ポインタとして返します。キャッシュが存在する場合はキャッシュを利用します。
	/// </summary>
	/// <typeparam name="T">読み込むデータの型。</typeparam>
	/// <param name="filename">読み込むデータファイルのファイル名。</param>
	/// <returns>データが正常に読み込まれた場合は T型のstd::vector配列 の共有ポインタ、失敗した場合は nullptr を返します。</returns>
	template <typename T>
	[[nodiscard(L"Unused Return Value")]] const std::shared_ptr<std::vector<T>> LoadData(_In_ const std::string& filename)
	{
		{
			auto it{ m_dataCache.find(filename) };
			if (it != m_dataCache.end()) return std::static_pointer_cast<std::vector<T>>(it->second);
		}

		auto newData{ std::make_shared<std::vector<T>>() };
		auto elementsNum{ size_t{NULL} };

		if (m_accessor.Load(filename, *newData, elementsNum)) 
		{
			m_dataCache[filename] = newData;
			return newData;
		}
		return nullptr;
	}

	/// <summary>
	/// 指定されたファイル名にデータを保存します。
	/// </summary>
	/// <typeparam name="_T">データベクター内の要素の型。</typeparam>
	/// <param name="filename">保存先のファイル名。</param>
	/// <param name="data">保存するデータのベクター。</param>
	/// <returns>保存に成功した場合は true、失敗した場合は false を返します。</returns>
	template <typename _T>
	[[nodiscard(L"Unused Return Value")]] const bool SaveData(_In_ const std::string& filename, _In_ const std::vector<_T>& data)
	{
		if (m_accessor.Save(filename, data)) 
		{
			m_dataCache[filename] = std::make_shared<std::vector<_T>>(data);
			return true;
		}
		return false;
	}

	/// <summary>
	/// データキャッシュをクリアします。
	/// </summary>
	/// <returns>なし（この関数は値を返しません）。</returns>
	inline auto ClearCache() noexcept { m_dataCache.clear(); }

	/// <summary>
	/// FlBinaryManager クラスのシングルトンインスタンスへの参照を返します。
	/// </summary>
	/// <returns>FlBinaryManager の唯一のインスタンスへの定数参照。</returns>
	//inline static auto& Instance() noexcept
	//{
	//	static auto instance{ FlBinaryManager{} };
	//	return instance;
	//}
	FlBinaryManager() = default;
	~FlBinaryManager() = default;

private:

	FlBinaryManager(const FlBinaryManager&)			   = delete;
	FlBinaryManager& operator=(const FlBinaryManager&) = delete;

	FlBinaryAccessor m_accessor;
	std::unordered_map<std::string, std::shared_ptr<void>> m_dataCache;
};
