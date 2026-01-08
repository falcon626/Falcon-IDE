#pragma once

class FlGuid
{
public:

	FlGuid() { NewGuid(); }

	// 新しいGUIDを作成する
	void NewGuid()
	{
		// すでにGUIDが設定されているかどうか
		auto prevId{ ToString() };
		auto status{ RPC_STATUS{} };

		do {
			status = UuidCreate(&m_guid);
			if (status != RPC_S_OK) _ASSERT_EXPR(false, "GUID Not Create");
		} while (status == RPC_S_UUID_LOCAL_ONLY);

		if (!prevId.empty()) m_replacedGuids[prevId] = ToString(); // 置き換え前のGUIDと置き換え後のGUIDを記録
	}

	std::string ToString() const
	{
		auto ret   { std::string{} };
		auto string{ RPC_CSTR{} };

		if (UuidToStringA(&m_guid, &string) == RPC_S_OK) ret = reinterpret_cast<char*>(string);

		return ret;
	}

	auto FromString(const std::string& strGuid)
	{
		auto status{ UuidFromStringA(reinterpret_cast<RPC_CSTR>(const_cast<char*>(strGuid.c_str())), &m_guid)};

		if (status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY) _ASSERT_EXPR(false, "GUID Not From String");
	}

	static const auto& GetReplacedGuid(const std::string& oldGuid)
	{
		auto it{ m_replacedGuids.find(oldGuid) };

		if (it != m_replacedGuids.end()) return it->second; // 置き換え後のGUIDを返す 
		return oldGuid; // 見つからなければ元のGUIDを返す
	}
private:
	UUID m_guid{};
	static std::map<std::string, std::string> m_replacedGuids; // 置き換え前のGUIDと置き換え後のGUIDのマップ
};