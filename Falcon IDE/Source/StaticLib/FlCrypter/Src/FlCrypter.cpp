#include "FlCrypter.h"

#include <sstream>
#include <iterator>
#include <iostream>

#define NOMINMAX
#include <windows.h>

namespace FlAssetProtector
{
	static DWORD GetFileAttr(const std::filesystem::path& path)
	{
#ifdef _WIN32
		DWORD attr = GetFileAttributesW(path.wstring().c_str());
		return (attr == INVALID_FILE_ATTRIBUTES) ? 0 : attr;
#else
		return 0;
#endif
	}

	static void ApplyAttrIfNeeded(const std::filesystem::path& path, DWORD srcAttr)
	{
#ifdef _WIN32
		DWORD attr = GetFileAttributesW(path.wstring().c_str());
		if (attr == INVALID_FILE_ATTRIBUTES) return;

		DWORD newAttr = attr;
		if (srcAttr & FILE_ATTRIBUTE_HIDDEN)   newAttr |= FILE_ATTRIBUTE_HIDDEN;
		if (srcAttr & FILE_ATTRIBUTE_READONLY) newAttr |= FILE_ATTRIBUTE_READONLY;
		if (srcAttr & FILE_ATTRIBUTE_SYSTEM)   newAttr |= FILE_ATTRIBUTE_SYSTEM;

		if (newAttr != attr)
			SetFileAttributesW(path.wstring().c_str(), newAttr);
#endif
	}

	static void CreateDirectoriesWithAttributes(
		const std::filesystem::path& srcRoot,
		const std::filesystem::path& dstRoot,
		const std::filesystem::path& relativeDir)
	{
		std::filesystem::path cur;

		for (const auto& part : relativeDir)
		{
			// "." や ".." を除外
			if (part == "." || part == "..") continue;

			cur /= part;

			auto srcPath = srcRoot / cur;
			auto dstPath = dstRoot / cur;

			// ファイルだったらディレクトリを作らない
			if (std::filesystem::exists(srcPath) &&
				std::filesystem::is_regular_file(srcPath))
			{
				break;
			}

			if (!std::filesystem::exists(dstPath))
			{
				std::error_code ec;
				std::filesystem::create_directory(dstPath, ec);

				if (ec) return; // 失敗時は静かに中断（例外なし）

				DWORD attr = GetFileAttr(srcPath);
				ApplyAttrIfNeeded(dstPath, attr);
			}
		}
	}

	bool ReadFileBinary(const std::filesystem::path& path, std::vector<uint8_t>& out)
	{
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs) return false;
		ifs.unsetf(std::ios::skipws);
		std::streampos size;
		ifs.seekg(0, std::ios::end);
		size = ifs.tellg();
		ifs.seekg(0, std::ios::beg);
		out.reserve(static_cast<size_t>(size));
		out.insert(out.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
		return true;
	}

	bool WriteFileBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data)
	{
		std::ofstream ofs(path, std::ios::binary);
		if (!ofs) return false;
		ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
		return true;
	}

	bool EncryptAssetFile(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath)
	{
		std::vector<uint8_t> raw;
		if (!ReadFileBinary(inputPath, raw)) return false;
		std::vector<uint8_t> encrypted;
		CryptoManager::EncryptXOR(raw, encrypted);

		std::filesystem::create_directories(outputPath.parent_path());
		return WriteFileBinary(outputPath, encrypted);
	}

	bool DecryptAssetFile(const std::filesystem::path& encryptedPath, std::vector<uint8_t>& outData)
	{
		std::vector<uint8_t> raw;
		if (!ReadFileBinary(encryptedPath, raw)) return false;
		return CryptoManager::DecryptXOR(raw, outData);
	}

	bool EncryptJsonFile(const std::filesystem::path& inputJson, const std::filesystem::path& outputPath)
	{
		std::ifstream ifs(inputJson);
		if (!ifs) return false;
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		std::string content = buffer.str();
		std::vector<uint8_t> plain(content.begin(), content.end());
		std::vector<uint8_t> encrypted;
		CryptoManager::EncryptXOR(plain, encrypted);

		std::filesystem::create_directories(outputPath.parent_path());
		return WriteFileBinary(outputPath, encrypted);
	}

	bool DecryptJsonToString(const std::filesystem::path& encryptedPath, std::string& outJsonStr)
	{
		std::vector<uint8_t> decrypted;
		if (!DecryptAssetFile(encryptedPath, decrypted)) return false;
		outJsonStr.assign(decrypted.begin(), decrypted.end());
		return true;
	}

	bool EncryptAllInDirectory(
		const std::filesystem::path& inputDir,
		const std::filesystem::path& outputDir)
	{
		if (!std::filesystem::exists(inputDir) ||
			!std::filesystem::is_directory(inputDir))
			return false;

		std::vector<std::filesystem::directory_entry> entries;

		for (const auto& e :
			std::filesystem::recursive_directory_iterator(inputDir))
		{
			entries.push_back(e);
		}

		bool result = true;

		for (const auto& entry : entries)
		{
			auto relative =
				entry.path().lexically_relative(inputDir);

			if (entry.is_directory())
			{
				CreateDirectoriesWithAttributes(
					inputDir,
					outputDir,
					entry.is_directory()
					? relative
					: relative.parent_path());

				continue;
			}

			if (!entry.is_regular_file()) continue;

			CreateDirectoriesWithAttributes(
				inputDir,
				outputDir,
				entry.is_directory()
				? relative
				: relative.parent_path());

			auto encName =
				CryptoManager::EncryptFilename(
					entry.path().filename().string());

			auto outPath =
				outputDir / relative.parent_path() / encName;

			result &= EncryptAssetFile(entry.path(), outPath);

			DWORD attr = GetFileAttr(entry.path());
			ApplyAttrIfNeeded(outPath, attr);
		}

		return result;
	}

	bool DecryptAllToOriginal(
		const std::filesystem::path& encryptedDir,
		const std::filesystem::path& outputDir)
	{
		if (!std::filesystem::exists(encryptedDir))
			return false;

		std::vector<std::filesystem::directory_entry> entries;
		for (const auto& e :
			std::filesystem::recursive_directory_iterator(encryptedDir))
		{
			entries.push_back(e);
		}

		bool result = true;

		for (const auto& entry : entries)
		{
			auto relative = entry.path().lexically_relative(encryptedDir);
			if (relative.empty() || relative == ".") continue;

			auto dir = entry.is_directory()
				? relative
				: relative.parent_path();

			if (dir.empty()) continue;

			std::error_code ec;
			std::filesystem::create_directories(outputDir / dir, ec);
			if (ec) result = false;
		}

		for (const auto& entry : entries)
		{
			if (!entry.is_regular_file()) continue;

			auto relative = entry.path().lexically_relative(encryptedDir);

			auto decName =
				CryptoManager::DecryptFilename(
					entry.path().filename().string());

			auto outPath =
				outputDir / relative.parent_path() / decName;

			std::vector<uint8_t> decrypted;
			if (!DecryptAssetFile(entry.path(), decrypted))
			{
				result = false;
				continue;
			}

			if (!WriteFileBinary(outPath, decrypted))
			{
				result = false;
				continue;
			}
		}

		for (const auto& entry : entries)
		{
			auto relative = entry.path().lexically_relative(encryptedDir);
			auto dstPath = outputDir / relative;

			if (!std::filesystem::exists(dstPath)) continue;

			DWORD attr = GetFileAttr(entry.path());
			ApplyAttrIfNeeded(dstPath, attr);
		}

		return result;
	}


	DecryptedInputStream::DecryptedInputStream(const std::filesystem::path& encryptedPath)
		: std::istream(nullptr)
	{
		std::vector<uint8_t> raw;
		if (!DecryptAssetFile(encryptedPath, raw)) return;
		m_buffer.assign(raw.begin(), raw.end());
		m_buf = std::make_unique<std::stringbuf>(std::string(m_buffer.begin(), m_buffer.end()), std::ios::in);
		rdbuf(m_buf.get());
		m_valid = true;
	}

	bool DecryptedInputStream::IsValid() const
	{
		return m_valid;
	}
}
