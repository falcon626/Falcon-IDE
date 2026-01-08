#pragma once

struct PendingReload {
    std::filesystem::path dllPath;
    std::chrono::steady_clock::time_point lastModified;
};

struct ScriptModule
{
    std::string moduleName{};
    std::filesystem::path folder{};
    std::filesystem::path originalDllPath{}; // 元のパス
    std::filesystem::path loadedDllPath{};   // 実際にロードしているコピーのパス
    FILETIME lastWriteTime{};
    HMODULE dll = nullptr;
};

class FlScriptModuleLoader
{
public:
    FlScriptModuleLoader(const std::string& root);
    ~FlScriptModuleLoader()
    {
        UnloadAll();
        if (m_upWatcher) m_upWatcher->Stop();
    }

    void Update() noexcept;

    void ScanModule(const std::filesystem::path& dllPath) noexcept;
    void DeleteModule(const std::filesystem::path& dllPath) noexcept;

private:
    std::string m_rootDir;
    std::vector<ScriptModule> m_modules;

    std::unique_ptr<FlFileWatcher> m_upWatcher;
    std::unordered_map<std::string, PendingReload> m_pendingReloads;

    void ScanModules() noexcept;
    void LoadAll() noexcept;
    void Load(ScriptModule& m) noexcept;
    void Unload(ScriptModule& m) noexcept;
    void UnloadAll() noexcept;
    void HotReload(ScriptModule& m) noexcept;
    bool GetLastWriteTime(const std::filesystem::path& p, FILETIME& out) noexcept;
};
