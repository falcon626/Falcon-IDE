#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <random>

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

			for (size_t i = 0; i < encryptedName.size(); i += 2)
			{
				std::string hexByte = encryptedName.substr(i, 2);
				uint8_t val = static_cast<uint8_t>(std::stoi(hexByte, nullptr, 16));
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
