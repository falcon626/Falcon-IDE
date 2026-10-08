#pragma once

#include <memory>
#include <mutex>

class FlGuid
{
public:

	FlGuid() { NewGuid(); }

	// 新しいGUIDを作成する
	void NewGuid()
	{
		const auto previous = ToString();
		UUID generated{};
		const auto status = UuidCreate(&generated);
		if (status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
		{
			_ASSERT_EXPR(false, L"GUID creation failed");
			return;
		}
		m_guid = generated;
		if (!previous.empty() && previous != "00000000-0000-0000-0000-000000000000")
		{
			std::lock_guard lock(m_replacedMutex);
			m_replacedGuids[previous] = ToString();
		}
	}

	std::string ToString() const
	{
		RPC_CSTR text{};
		if (UuidToStringA(&m_guid, &text) != RPC_S_OK) return {};
		const auto freeText = [](unsigned char* value) { RpcStringFreeA(&value); };
		const std::unique_ptr<unsigned char, decltype(freeText)> owner(text, freeText);
		return reinterpret_cast<const char*>(text);
	}

	bool FromString(const std::string& strGuid)
	{
		if (strGuid.empty() || strGuid.find('\0') != std::string::npos) return false;
		UUID parsed{};
		const auto status = UuidFromStringA(reinterpret_cast<RPC_CSTR>(const_cast<char*>(strGuid.c_str())), &parsed);
		if (status != RPC_S_OK) return false;
		m_guid = parsed;
		return true;
	}

	static std::string GetReplacedGuid(const std::string& oldGuid)
	{
		std::lock_guard lock(m_replacedMutex);
		const auto it = m_replacedGuids.find(oldGuid);
		return it != m_replacedGuids.end() ? it->second : oldGuid;
	}

private:
	inline static std::mutex m_replacedMutex;
	UUID m_guid{};
	static std::map<std::string, std::string> m_replacedGuids; // 置き換え前のGUIDと置き換え後のGUIDのマップ
};