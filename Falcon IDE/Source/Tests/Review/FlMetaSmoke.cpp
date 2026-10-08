#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "../../Src/Framework/Resource/Json/json.hpp"

namespace Def { constexpr int IntZero = 0, IntOne = 1; constexpr unsigned UIntZero = 0, BitMaskPos3 = 4; }
struct SmokeLogger {
    template<class... Args> void AddErrorLog(const std::string& text, Args...) { std::cerr << text << "; Win32=" << GetLastError() << '\n'; }
    template<class... Args> void AddErrorLogU8(const std::u8string& text, Args...) { std::cerr << reinterpret_cast<const char*>(text.c_str()) << "; Win32=" << GetLastError() << '\n'; }
    template<class... Args> void AddChangeLogU8(const std::u8string&, Args...) {}
};
struct FlEditorAdministrator {
    SmokeLogger logger;
    static auto& Instance() { static FlEditorAdministrator value; return value; }
    auto* GetLogger() { return &logger; }
};
struct FlGuid {
    std::string value;
    FlGuid() { NewGuid(); }
    void NewGuid() { static std::atomic<unsigned> next{}; value = "smoke-guid-" + std::to_string(++next); }
    auto ToString() const { return value; }
};

// Current production sources, with their includes relocated by the runner.
#include "FlMetaJson.inc"
#include "FlMetaWatcher.inc"
#include "FlMetaProduction.inc"

#define CHECK(x) do { if (!(x)) { std::cerr << "Meta check failed at " << __LINE__ << ": " #x << '\n'; std::exit(1); } } while (false)

static std::string Read(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

int main(int argc, char** argv)
{
    CHECK(argc == 2);
    int utilityValue = 0;
    FlJsonUtility::GetValue(nlohmann::json{ { "value", 1 } }, "value", [&](const nlohmann::json& value) { utilityValue += value.get<int>(); });
    FlJsonUtility::GetArray(nlohmann::json{ { "values", { 2 } } }, "values", [&](uint32_t, const nlohmann::json& value) { utilityValue += value.get<int>(); });
    CHECK(utilityValue == 3);
    const auto root = std::filesystem::path(argv[1]) / "MetaSmokeData";
    std::filesystem::create_directories(root);
    CHECK(FlJsonUtility::Serialize(nlohmann::json{ { "probe", true } }, root / "probe.json"));
    std::filesystem::remove(root / "probe.json");
    const auto asset = root / "script.cxx";
    { std::ofstream file(asset); file << "version 1"; }
    const auto firstTime = std::chrono::time_point_cast<std::chrono::seconds>(std::filesystem::file_time_type::clock::now()) + std::chrono::milliseconds(100);
    const auto nextTime = firstTime + std::chrono::milliseconds(100);
    std::filesystem::last_write_time(asset, firstTime);
    const auto metaPath = root / ".FlMeta" / "script.cxx.flmeta";

    FlMetaFileManager first;
    first.StartMonitoring(root.string());
    first.StopMonitoring();
    const auto guid = first.FindGuidByAsset(asset);
    if (!guid) std::cerr << "Missing GUID: " << metaPath << "; exists=" << std::filesystem::exists(metaPath) << "; metadata=" << Read(metaPath) << "; map=" << nlohmann::json(first.GetGuidMap()).dump() << '\n';
    CHECK(guid && !guid->empty());
    CHECK(first.ResetAssetChangeFlag(asset, firstTime));
    CHECK(!first.IsAssetChanged(asset));

    // An unchanged startup scan must restore the map, not just skip its metadata.
    FlMetaFileManager restarted;
    restarted.StartMonitoring(root.string());
    restarted.StopMonitoring();
    CHECK(restarted.FindGuidByAsset(asset) == guid);
    CHECK(restarted.FindAssetByGuid(*guid) == asset.string());
    auto snapshot = restarted.GetGuidMap();
    snapshot.clear();
    CHECK(restarted.GetGuidMap().contains(*guid));
    CHECK(!restarted.IsAssetChanged(asset));

    // These writes occur in the same second; tick precision must still detect them.
    std::filesystem::last_write_time(asset, nextTime);
    restarted.StartMonitoring(root.string());
    restarted.StopMonitoring();
    CHECK(restarted.IsAssetChanged(asset));
    CHECK(!restarted.ResetAssetChangeFlag(asset, firstTime));
    CHECK(restarted.IsAssetChanged(asset));
    CHECK(restarted.ResetAssetChangeFlag(asset, nextTime));
    CHECK(!restarted.IsAssetChanged(asset));
    CHECK(restarted.FindGuidByAsset(asset) == guid);

    auto metadata = nlohmann::json{};
    CHECK(FlJsonUtility::Deserialize(metadata, metaPath));
    metadata["lastModified"] = 123;
    metadata["loadFlag"] = "bad type";
    metadata["isChanged"] = "bad type";
    metadata.erase("lastWriteTicks"); // Exercise the legacy timestamp fallback too.
    CHECK(FlJsonUtility::Serialize(metadata, metaPath));
    CHECK(!restarted.IsAssetChanged(asset));
    restarted.IncrementLoadFlag(asset.string());
    CHECK(restarted.FindGuidByAsset(asset) == guid);
    restarted.StartMonitoring(root.string());
    restarted.StopMonitoring();
    CHECK(restarted.FindGuidByAsset(asset) == guid);
    CHECK(restarted.IsAssetChanged(asset));
    CHECK(FlJsonUtility::Deserialize(metadata, metaPath));
    CHECK(metadata["lastModified"].is_string());
    CHECK(metadata["loadFlag"].is_boolean() && metadata["isChanged"].is_boolean());

    // A syntactically broken metadata file must remain available for repair.
    { std::ofstream file(metaPath); file << "{broken-metadata"; }
    const auto broken = Read(metaPath);
    restarted.StartMonitoring(root.string());
    restarted.StopMonitoring();
    CHECK(Read(metaPath) == broken);
    CHECK(!restarted.ResetAssetChangeFlag(asset, nextTime));
    CHECK(Read(metaPath) == broken);
    CHECK(FlJsonUtility::Serialize(metadata, metaPath));

    // Re-enter StartMonitoring with a live production watcher (Stop precedes its lock).
    restarted.StartMonitoring(root.string());
    auto restart = std::async(std::launch::async, [&] { restarted.StartMonitoring(root.string()); });
    CHECK(restart.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    restart.get();
    restarted.StopMonitoring();
    CHECK(restarted.FindGuidByAsset(asset) == guid);
    std::filesystem::remove_all(root);
    std::cout << "Meta smoke PASS: restart map, snapshot, tick precision, reset guard, bad metadata, watcher restart\n";
}
