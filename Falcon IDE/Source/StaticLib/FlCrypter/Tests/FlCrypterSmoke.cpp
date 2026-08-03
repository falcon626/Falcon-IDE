#include "../Src/FlCrypter.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
	#define CHECK(condition) \
		do { if (!(condition)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #condition << '\n'; std::exit(EXIT_FAILURE); } } while (false)

	std::vector<char> ReadAll(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		return { std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
	}

	void WriteAll(const std::filesystem::path& path, const std::vector<char>& data)
	{
		std::ofstream stream{ path, std::ios::binary };
		stream.write(data.data(), static_cast<std::streamsize>(data.size()));
		CHECK(stream);
	}

	void ExpectRejectedEntry(
		const std::filesystem::path& root,
		const std::filesystem::path& source,
		const std::string& encryptedName,
		const std::string& caseName,
		const std::vector<char>& sentinel)
	{
		const auto caseRoot = root / caseName;
		const auto encrypted = caseRoot / "encrypted";
		const auto output = caseRoot / "output";
		std::filesystem::create_directories(encrypted);
		std::filesystem::create_directories(output);
		WriteAll(output / "keep.bin", sentinel);

		CHECK(FlAssetProtector::EncryptAssetFile(source, encrypted / encryptedName));
		CHECK(!FlAssetProtector::DecryptAllToOriginal(encrypted, output));
		CHECK(ReadAll(output / "keep.bin") == sentinel);
	}
}

int main()
{
	using namespace FlAssetProtector;

	const auto unique = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
	const auto root = std::filesystem::temp_directory_path() / ("FalconCrypterSmoke-" + unique);
	const auto input = root / "input";
	const auto encrypted = root / "encrypted";
	const auto output = root / "output";

	std::filesystem::create_directories(input);
	const auto source = input / "sample.bin";
	const std::vector<char> expected{ '\0', '\x01', 'F', 'a', 'l', 'c', 'o', 'n', '\x7f' };
	WriteAll(source, expected);

	CHECK(EncryptAllInDirectory(input, encrypted));
	CHECK(DecryptAllToOriginal(encrypted, output));
	CHECK(ReadAll(output / source.filename()) == expected);

	const std::vector<char> sentinel{ 'k', 'e', 'e', 'p' };
	ExpectRejectedEntry(root, source, CryptoManager::EncryptFilename("..\\escaped.bin"), "traversal", sentinel);
	ExpectRejectedEntry(root, source, CryptoManager::EncryptFilename(std::string{ "file\0evil", 9 }), "embedded-nul", sentinel);
	ExpectRejectedEntry(root, source, CryptoManager::EncryptFilename("CON.txt"), "device", sentinel);
	ExpectRejectedEntry(root, source, "0G", "malformed-hex", sentinel);
	CHECK(!std::filesystem::exists(root / "escaped.bin"));

	const auto collisionEncrypted = root / "collision" / "encrypted";
	const auto collisionOutput = root / "collision" / "output";
	std::filesystem::create_directories(collisionEncrypted);
	std::filesystem::create_directories(collisionOutput);
	WriteAll(collisionOutput / "keep.bin", sentinel);
	CHECK(EncryptAssetFile(source, collisionEncrypted / CryptoManager::EncryptFilename("A")));
	CHECK(EncryptAssetFile(source, collisionEncrypted / CryptoManager::EncryptFilename("a")));
	CHECK(!DecryptAllToOriginal(collisionEncrypted, collisionOutput));
	CHECK(ReadAll(collisionOutput / "keep.bin") == sentinel);

	const auto existing = root / "existing.bin";
	WriteAll(existing, sentinel);
	CHECK(!EncryptAssetFile(root / "missing.bin", existing));
	CHECK(ReadAll(existing) == sentinel);

	auto nulOutput = existing.string();
	nulOutput.append("\0evil", 5);
	CHECK(!EncryptAssetFile(source, std::filesystem::path{ nulOutput }));
	CHECK(ReadAll(existing) == sentinel);

	std::filesystem::remove_all(root);
}
