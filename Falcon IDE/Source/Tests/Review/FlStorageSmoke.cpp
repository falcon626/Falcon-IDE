#define NOMINMAX
#include <windows.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include "../../Src/Framework/Resource/Json/json.hpp"

// Only these constants are needed by the production JSON helpers.
namespace Def { constexpr unsigned UIntZero = 0; constexpr unsigned BitMaskPos3 = 4; }
#include "../../Src/Framework/Utility/FlUtilityJson.hxx"

#define CHECK(x) do { if (!(x)) { std::cerr << "Storage check failed at " << __LINE__ << ": " #x << '\n'; std::exit(1); } } while (false)

static std::string Read(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() /
        ("FalconStorageSmoke-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto path = root / "scene.json";
    const nlohmann::json original{ { "value", 42 } };
    CHECK(FlJsonUtility::Serialize(original, path));
    const auto before = Read(path);
    auto parsed = nlohmann::json{};
    CHECK(FlJsonUtility::Deserialize(parsed, path) && parsed == original);
    int value = 0;
    FlJsonUtility::GetValue(parsed, "value", [&](const nlohmann::json& entry) { value = entry.get<int>(); });
    CHECK(value == 42);
    FlJsonUtility::GetArray(nlohmann::json{{ "values", { 1, 2 } }}, "values", [&](uint32_t, const nlohmann::json& entry) { value += entry.get<int>(); });
    CHECK(value == 45);

    const auto locked = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(locked != INVALID_HANDLE_VALUE);
    CHECK(!FlJsonUtility::Serialize(nlohmann::json{ { "value", 99 } }, path));
    CHECK(Read(path) == before);
    CloseHandle(locked);
    CHECK(!FlJsonUtility::Serialize(nlohmann::json(std::string(1, '\xff')), path));
    CHECK(Read(path) == before);
    CHECK(!FlJsonUtility::Serialize(original, root / "missing" / "file.json"));
    CHECK(!FlJsonUtility::Serialize(original, root));
    { std::ofstream file(root / "broken.json"); file << "{"; }
    CHECK(!FlJsonUtility::Deserialize(parsed, root / "broken.json"));
    CHECK(parsed == original);

    const auto assets = root / "Assets";
    const auto archive = root / "CryptedAssets";
    std::filesystem::create_directories(assets);
    CHECK(FlJsonUtility::Serialize(original, assets / "state.json"));
    CHECK(FlAssetProtector::EncryptAllInDirectory(assets, archive));
    const nlohmann::json latest{ { "value", 100 } };
    CHECK(FlJsonUtility::Serialize(latest, assets / "state.json"));
    CHECK(FlAssetProtector::RestoreAssetsIfMissing(archive, assets));
    CHECK(FlJsonUtility::Deserialize(parsed, assets / "state.json") && parsed == latest);
    CHECK(std::filesystem::is_directory(archive));
    std::filesystem::remove_all(assets);
    CHECK(FlAssetProtector::RestoreAssetsIfMissing(archive, assets));
    CHECK(FlJsonUtility::Deserialize(parsed, assets / "state.json") && parsed == original);
    CHECK(std::filesystem::is_directory(archive));
    CHECK(!FlAssetProtector::RestoreAssetsIfMissing(root / "missing", root / "no-assets"));

    for (const auto& entry : std::filesystem::directory_iterator(root))
        CHECK(!entry.path().filename().string().starts_with(".tmp-"));
    std::filesystem::remove_all(root);
    std::cout << "Storage smoke PASS\n";
}
