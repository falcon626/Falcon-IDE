#include "FlCrypter.h"

#include <limits>
#include <algorithm>
#include <cctype>
#include <string_view>

#define NOMINMAX
#include <windows.h>

namespace FlAssetProtector
{
	static bool IsSafeFilename(const std::string& name)
	{
		if (name.empty() || name.back() == '.' || name.back() == ' ')
			return false;

		for (const unsigned char ch : name)
		{
			if (ch < 32 || std::string_view{ "<>:\"/\\|?*" }.find(static_cast<char>(ch)) != std::string_view::npos)
				return false;
		}

		auto deviceName = name.substr(0, name.find('.'));
		for (auto& ch : deviceName)
			ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

		const bool reservedDevice = deviceName == "CON" || deviceName == "PRN"
			|| deviceName == "AUX" || deviceName == "NUL"
			|| (deviceName.size() == 4
				&& ((deviceName.starts_with("COM") || deviceName.starts_with("LPT"))
					&& deviceName.back() >= '1' && deviceName.back() <= '9'));
		if (reservedDevice) return false;

		const std::filesystem::path path{ name };
		return path != "."
			&& path != ".."
			&& !path.is_absolute()
			&& !path.has_root_path()
			&& !path.has_parent_path()
			&& path.filename() == path;
	}

	static bool IsWithinRoot(
		const std::filesystem::path& root,
		const std::filesystem::path& candidate)
	{
		std::error_code ec;
		const auto canonicalRoot = std::filesystem::weakly_canonical(root, ec);
		if (ec) return false;

		const auto canonicalCandidate = std::filesystem::weakly_canonical(candidate, ec);
		if (ec) return false;

		auto candidatePart = canonicalCandidate.begin();
		for (auto rootPart = canonicalRoot.begin(); rootPart != canonicalRoot.end(); ++rootPart, ++candidatePart)
		{
			if (candidatePart == canonicalCandidate.end() || *candidatePart != *rootPart)
				return false;
		}
		return true;
	}

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

	static bool CreateDirectoriesWithAttributes(
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
			std::error_code ec;
			if (std::filesystem::is_regular_file(srcPath, ec))
			{
				break;
			}
			if (ec) return false;

			if (!std::filesystem::exists(dstPath, ec))
			{
				if (ec) return false;
				std::filesystem::create_directory(dstPath, ec);

				if (ec) return false;

				DWORD attr = GetFileAttr(srcPath);
				ApplyAttrIfNeeded(dstPath, attr);
			}
		}

		return true;
	}

	static bool CreateTransactionPaths(
		const std::filesystem::path& outputDir,
		std::filesystem::path& stagingDir,
		std::filesystem::path& backupDir)
	{
		auto parent = outputDir.parent_path();
		if (parent.empty()) parent = ".";

		std::error_code ec;
		std::filesystem::create_directories(parent, ec);
		if (ec) return false;

		const auto base = outputDir.filename().wstring();
		const auto token = std::to_wstring(GetCurrentProcessId())
			+ L"-" + std::to_wstring(GetTickCount64());

		for (unsigned int attempt = 0; attempt < 32; ++attempt)
		{
			const auto suffix = token + L"-" + std::to_wstring(attempt);
			stagingDir = parent / (base + L".stage-" + suffix);
			backupDir = parent / (base + L".backup-" + suffix);

			ec.clear();
			if (std::filesystem::create_directory(stagingDir, ec))
				return true;
			if (ec && ec != std::errc::file_exists)
				return false;
		}

		return false;
	}

	static bool CommitDirectory(
		const std::filesystem::path& stagingDir,
		const std::filesystem::path& outputDir,
		const std::filesystem::path& backupDir)
	{
		std::error_code ec;
		const bool hadOutput = std::filesystem::exists(outputDir, ec);
		if (ec) return false;

		if (hadOutput)
		{
			std::filesystem::rename(outputDir, backupDir, ec);
			if (ec) return false;
		}

		std::filesystem::rename(stagingDir, outputDir, ec);
		if (ec)
		{
			if (hadOutput)
			{
				std::error_code restoreError;
				std::filesystem::rename(backupDir, outputDir, restoreError);
			}
			return false;
		}

		if (hadOutput)
		{
			std::error_code cleanupError;
			std::filesystem::remove_all(backupDir, cleanupError);
		}

		return true;
	}

	static bool CollectEntries(
		const std::filesystem::path& root,
		std::vector<std::filesystem::directory_entry>& entries)
	{
		std::error_code ec;
		std::filesystem::recursive_directory_iterator it{ root, std::filesystem::directory_options::none, ec };
		const std::filesystem::recursive_directory_iterator end;
		if (ec) return false;

		for (; it != end; it.increment(ec))
		{
			if (ec) return false;
			entries.push_back(*it);
		}

		return !ec;
	}

	class StagingDirectoryCleanup
	{
	public:
		explicit StagingDirectoryCleanup(const std::filesystem::path& path) noexcept
			: m_path{ path }
		{
		}

		~StagingDirectoryCleanup()
		{
			try
			{
				if (m_path.empty()) return;
				std::error_code ec;
				std::filesystem::remove_all(m_path, ec);
			}
			catch (...)
			{
			}
		}

	private:
		const std::filesystem::path& m_path;
	};

	bool ReadFileBinary(const std::filesystem::path& path, std::vector<uint8_t>& out)
	{
		try
		{
			out.clear();
			std::ifstream ifs(path, std::ios::binary | std::ios::ate);
			if (!ifs) return false;

			const auto end = ifs.tellg();
			if (end < 0 || static_cast<uintmax_t>(end) > std::numeric_limits<size_t>::max()
				|| static_cast<uintmax_t>(end) > static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max()))
				return false;

			out.resize(static_cast<size_t>(end));
			ifs.seekg(0, std::ios::beg);
			if (!ifs) return false;

			if (!out.empty())
				ifs.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));

			if (!ifs || static_cast<size_t>(ifs.gcount()) != out.size())
			{
				out.clear();
				return false;
			}

			return true;
		}
		catch (...)
		{
			out.clear();
			return false;
		}
	}

	bool WriteFileBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data)
	{
		try
		{
			const auto nativePath = path.native();
			if (std::find(nativePath.begin(), nativePath.end(), std::filesystem::path::value_type{}) != nativePath.end())
				return false;
			if (data.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
				return false;

			const auto tempPath = path.parent_path() /
				(L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-"
					+ std::to_wstring(GetCurrentThreadId()) + L"-" + std::to_wstring(GetTickCount64()));

			std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
			if (!ofs) return false;

			ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
			ofs.flush();
			if (!ofs)
			{
				ofs.close();
				std::error_code ec;
				std::filesystem::remove(tempPath, ec);
				return false;
			}

			ofs.close();
			std::error_code ec;
			if (!ofs || std::filesystem::file_size(tempPath, ec) != data.size() || ec)
			{
				std::filesystem::remove(tempPath, ec);
				return false;
			}

#ifdef _WIN32
			if (!MoveFileExW(tempPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			{
				std::filesystem::remove(tempPath, ec);
				return false;
			}
#else
			std::filesystem::rename(tempPath, path, ec);
			if (ec)
			{
				std::filesystem::remove(tempPath, ec);
				return false;
			}
#endif

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool EncryptAssetFile(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath)
	{
		try
		{
			std::vector<uint8_t> raw;
			if (!ReadFileBinary(inputPath, raw)) return false;
			std::vector<uint8_t> encrypted;
			if (!CryptoManager::EncryptXOR(raw, encrypted)) return false;

			const auto parent = outputPath.parent_path();
			if (!parent.empty())
			{
				std::error_code ec;
				std::filesystem::create_directories(parent, ec);
				if (ec) return false;
			}
			return WriteFileBinary(outputPath, encrypted);
		}
		catch (...)
		{
			return false;
		}
	}

	bool DecryptAssetFile(const std::filesystem::path& encryptedPath, std::vector<uint8_t>& outData)
	{
		try
		{
			std::vector<uint8_t> raw;
			if (!ReadFileBinary(encryptedPath, raw)) return false;
			return CryptoManager::DecryptXOR(raw, outData);
		}
		catch (...)
		{
			outData.clear();
			return false;
		}
	}

	bool EncryptJsonFile(const std::filesystem::path& inputJson, const std::filesystem::path& outputPath)
	{
		return EncryptAssetFile(inputJson, outputPath);
	}

	bool DecryptJsonToString(const std::filesystem::path& encryptedPath, std::string& outJsonStr)
	{
		try
		{
			std::vector<uint8_t> decrypted;
			if (!DecryptAssetFile(encryptedPath, decrypted)) return false;
			outJsonStr.assign(decrypted.begin(), decrypted.end());
			return true;
		}
		catch (...)
		{
			outJsonStr.clear();
			return false;
		}
	}

	bool EncryptAllInDirectory(
		const std::filesystem::path& inputDir,
		const std::filesystem::path& outputDir)
	{
		std::filesystem::path stagingDir;
		std::filesystem::path backupDir;
		const StagingDirectoryCleanup cleanup{ stagingDir };

		try
		{
			std::error_code ec;
			if (outputDir.empty() || outputDir.filename().empty()
				|| !std::filesystem::is_directory(inputDir, ec) || ec)
				return false;

			if (IsWithinRoot(inputDir, outputDir) || IsWithinRoot(outputDir, inputDir))
				return false;

			std::vector<std::filesystem::directory_entry> entries;
			if (!CollectEntries(inputDir, entries)
				|| !CreateTransactionPaths(outputDir, stagingDir, backupDir))
				return false;

			for (const auto& entry : entries)
			{
				ec.clear();
				if (entry.is_symlink(ec) || ec)
					return false;

				const auto relative = entry.path().lexically_relative(inputDir);
				const bool isDirectory = entry.is_directory(ec);
				if (ec) return false;

				if (isDirectory)
				{
					if (!CreateDirectoriesWithAttributes(inputDir, stagingDir, relative))
						return false;
					continue;
				}

				if (!entry.is_regular_file(ec) || ec)
					return false;

				if (!CreateDirectoriesWithAttributes(inputDir, stagingDir, relative.parent_path()))
					return false;

				const auto encName = CryptoManager::EncryptFilename(entry.path().filename().string());
				const auto outPath = stagingDir / relative.parent_path() / encName;
				if (!EncryptAssetFile(entry.path(), outPath))
					return false;

				ApplyAttrIfNeeded(outPath, GetFileAttr(entry.path()));
			}

			if (!CommitDirectory(stagingDir, outputDir, backupDir))
				return false;

			return true;
		}
		catch (...)
		{
		}

		return false;
	}

	bool DecryptAllToOriginal(
		const std::filesystem::path& encryptedDir,
		const std::filesystem::path& outputDir)
	{
		std::filesystem::path stagingDir;
		std::filesystem::path backupDir;
		const StagingDirectoryCleanup cleanup{ stagingDir };

		try
		{
			std::error_code ec;
			if (outputDir.empty() || outputDir.filename().empty()
				|| !std::filesystem::is_directory(encryptedDir, ec) || ec)
				return false;

			if (IsWithinRoot(encryptedDir, outputDir) || IsWithinRoot(outputDir, encryptedDir))
				return false;

			std::vector<std::filesystem::directory_entry> entries;
			if (!CollectEntries(encryptedDir, entries)
				|| !CreateTransactionPaths(outputDir, stagingDir, backupDir))
				return false;

			for (const auto& entry : entries)
			{
				ec.clear();
				if (entry.is_symlink(ec) || ec)
					return false;

				const auto relative = entry.path().lexically_relative(encryptedDir);
				const bool isDirectory = entry.is_directory(ec);
				if (ec) return false;

				const auto dir = isDirectory ? relative : relative.parent_path();
				if (!dir.empty() && !CreateDirectoriesWithAttributes(encryptedDir, stagingDir, dir))
					return false;
			}

			for (const auto& entry : entries)
			{
				ec.clear();
				if (entry.is_directory(ec)) continue;
				if (ec || !entry.is_regular_file(ec) || ec)
					return false;

				const auto relative = entry.path().lexically_relative(encryptedDir);
				const auto decName = CryptoManager::DecryptFilename(entry.path().filename().string());
				if (!IsSafeFilename(decName))
					return false;

				const auto outPath =
					(stagingDir / relative.parent_path() / decName).lexically_normal();
				if (!IsWithinRoot(stagingDir, outPath))
					return false;

				ec.clear();
				if (std::filesystem::exists(outPath, ec) || ec)
					return false;

				std::vector<uint8_t> decrypted;
				if (!DecryptAssetFile(entry.path(), decrypted)
					|| !WriteFileBinary(outPath, decrypted))
					return false;

				ApplyAttrIfNeeded(outPath, GetFileAttr(entry.path()));
			}

			return CommitDirectory(stagingDir, outputDir, backupDir);
		}
		catch (...)
		{
			return false;
		}
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
