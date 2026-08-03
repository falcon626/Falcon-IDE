#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace FlAssetProtector
{
	constexpr uint8_t XOR_KEY = 0xF1;

	class CryptoManager
	{
	public:
		static bool EncryptXOR(const std::vector<uint8_t>& plaintext, std::vector<uint8_t>& ciphertext)
		{
			ciphertext.resize(plaintext.size());
			for (size_t i = 0; i < plaintext.size(); ++i)
			{
				ciphertext[i] = plaintext[i] ^ XOR_KEY;
			}
			return true;
		}

		static bool DecryptXOR(const std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& plaintext)
		{
			plaintext.resize(ciphertext.size());
			for (size_t i = 0; i < ciphertext.size(); ++i)
			{
				plaintext[i] = ciphertext[i] ^ XOR_KEY;
			}
			return true;
		}

		static std::string EncryptFilename(const std::string& original)
		{
			std::ostringstream oss;
			for (char ch : original)
			{
				uint8_t encryptedChar = static_cast<uint8_t>(ch ^ XOR_KEY);
				oss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << static_cast<int>(encryptedChar);
			}
			return oss.str();
		}

		static std::string DecryptFilename(const std::string& encryptedName)
		{
			std::string result;
			if (encryptedName.size() % 2 != 0) return "";
			result.reserve(encryptedName.size() / 2);

			const auto hexValue = [](const char ch) noexcept
				{
					if (ch >= '0' && ch <= '9') return ch - '0';
					if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
					if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
					return -1;
				};

			for (size_t i = 0; i < encryptedName.size(); i += 2)
			{
				const int high = hexValue(encryptedName[i]);
				const int low = hexValue(encryptedName[i + 1]);
				if (high < 0 || low < 0) return "";
				const auto val = static_cast<uint8_t>((high << 4) | low);
				result.push_back(static_cast<char>(val ^ XOR_KEY));
			}
			return result;
		}
	};

	bool EncryptAssetFile(const std::filesystem::path& inputPath, const std::filesystem::path& outputDir);
	bool DecryptAssetFile(const std::filesystem::path& encryptedPath, std::vector<uint8_t>& outData);

	bool EncryptJsonFile(const std::filesystem::path& inputJson, const std::filesystem::path& outputDir);
	bool DecryptJsonToString(const std::filesystem::path& encryptedPath, std::string& outJsonStr);

	bool EncryptAllInDirectory(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir);
	bool DecryptAllToOriginal(const std::filesystem::path& encryptedDir, const std::filesystem::path& outputDir);

	class DecryptedInputStream : public std::istream
	{
	public:
		explicit DecryptedInputStream(const std::filesystem::path& encryptedPath);
		bool IsValid() const;
	private:
		std::vector<char> m_buffer;
		std::unique_ptr<std::streambuf> m_buf;
		bool m_valid = false;
	};
} // namespace FlAssetProtector
