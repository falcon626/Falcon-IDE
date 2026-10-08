// Standalone check: cl /std:c++20 /EHsc /W4 FlLogWatcherSmoke.cpp
// Rendering, formatting and the app singleton are excluded; storage and the
// watcher worker below are the production implementations.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../Src/Framework/ImGui/imgui.h"

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "FAIL at %d: %s\n", __LINE__, #condition); std::exit(1); } } while (false)

namespace Math
{
    struct Color
    {
        float r, g, b, a;
    };
}
namespace Def
{
    constexpr float Half = 0.5f, FloatOne = 1.0f, FloatZero = 0.0f;
    constexpr int IntZero = 0;
}
struct FlChronus
{
    static std::string now_iso8601() { return "test"; }
};
namespace Str
{
    template<class... Args> std::string FormatString(const char* format, Args... args)
    {
        char buffer[128]{};
        std::snprintf(buffer, sizeof(buffer), format, args...);
        return buffer;
    }
    template<class... Args> std::wstring FormatStringW(const wchar_t* format, Args...) { return format; }
    template<class... Args> std::u8string FormatStringU8(const char8_t* format, Args...) { return format; }
    std::string U8StringToStringSafe(const std::u8string& text) { return { text.begin(), text.end() }; }
}
std::string wide_to_ansi(const std::wstring& text)
{
    std::string result;
    for (const auto character : text) result.push_back(static_cast<char>(character));
    return result;
}

// Inspect snapshots without introducing a public test-only production API.
#define private public
#include "../../Src/Framework/ImGui/Editor/FlLogEditor.h"
#undef private

class FlEditorAdministrator
{
public:
    static auto& Instance() { static FlEditorAdministrator instance; return instance; }
    auto GetLogger() { return &m_log; }
private:
    FlLogEditor m_log;
};

#include "../../Src/Framework/System/Watcher/FlFileWatcher.cpp"

template<class Predicate> bool WaitUntil(Predicate ready)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(6);
    while (!ready() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return ready();
}

int main()
{
    auto& log = *FlEditorAdministrator::Instance().GetLogger();
    log.AddLog("literal %s %n 100%");
    log.AddLogW(L"wide 100%");
    log.AddLogU8(u8"utf8 100%");
    auto first = log.TakeSnapshot();
    CHECK(first.first.size() == 3 && first.second);
    CHECK(first.first.front().text.ends_with("literal %s %n 100%"));
    CHECK(!log.TakeSnapshot().second);
    log.Clear();

    std::atomic<bool> done{};
    std::thread reader([&] {
        while (!done) {
            const auto snapshot = log.TakeSnapshot();
            CHECK(snapshot.first.size() <= 4000);
        }
    });
    std::vector<std::thread> writers;
    for (int thread = 0; thread < 4; ++thread)
        writers.emplace_back([&] { for (int i = 0; i < 1000; ++i) log.AddLog("message %d", i); });
    for (auto& writer : writers) writer.join();
    done = true;
    reader.join();
    CHECK(log.TakeSnapshot().first.size() == 4000);
    log.Clear();
    CHECK(log.TakeSnapshot().first.empty());

    const auto root = std::filesystem::temp_directory_path() /
        ("FalconLogWatcherSmoke-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    FlFileWatcher watcher;
    watcher.SetPathAndInterval(root, std::chrono::seconds(1));
    std::atomic<int> createdAttempts{}, erasedAttempts{}, created{}, erased{};
    const auto callback = [&](const auto&, FlFileWatcher::FileStatus status) {
        if (status == FlFileWatcher::FileStatus::Created) {
            if (++createdAttempts == 1) throw std::runtime_error("created callback failure");
            ++created;
        }
        if (status == FlFileWatcher::FileStatus::Erased) {
            if (++erasedAttempts == 1) throw std::runtime_error("erased callback failure");
            ++erased;
        }
    };
    watcher.Start(callback);
    watcher.Start(callback); // A restart must join the previous worker.
    const auto file = root / "sample.txt";
    { std::ofstream output(file); output << "sample"; CHECK(output); }
    CHECK(WaitUntil([&] { return created.load() == 1; }));
    CHECK(createdAttempts == 2);
    CHECK(std::filesystem::remove(file));
    CHECK(WaitUntil([&] { return erased.load() == 1; }));
    CHECK(erasedAttempts == 2);
    watcher.Stop();
    CHECK(log.TakeSnapshot().first.size() == 2);
    std::filesystem::remove_all(root);
    std::puts("FlLogWatcherSmoke: PASS");
}
