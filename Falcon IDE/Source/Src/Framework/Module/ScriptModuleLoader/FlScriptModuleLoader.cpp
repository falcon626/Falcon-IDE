#include "FlScriptModuleLoader.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

#include "../../Application/Application.h"

FlScriptModuleLoader::FlScriptModuleLoader(const std::string& root)
    : m_rootDir{ root }
    , m_upWatcher{ std::make_unique<FlFileWatcher>() }
{
    // 初回スキャン ＆ ロード
    ScanModules();
    LoadAll();

    // -------------------------
    // FlFileWatcher の設定開始
    // -------------------------
    m_upWatcher->SetPathAndInterval(
        m_rootDir,
        FlChronus::sec(Def::UIntOne)
    );

    m_upWatcher->Start(
        [this](const std::filesystem::path& path, FlFileWatcher::FileStatus status)
        {
            if (path.extension() != ".dll")
                return;

            if (status == FlFileWatcher::FileStatus::Created || status == FlFileWatcher::FileStatus::Modified)
            {
                auto key = path.string();
                m_pendingReloads[key] = { path, std::chrono::steady_clock::now() };
            }

            else if (status == FlFileWatcher::FileStatus::Erased)
            {
                DeleteModule(path);
            }
        }
    );
}

static FlRuntimeAPI* CreateRuntimeAPI() noexcept // Wrap
{
    static auto api{ FlRuntimeAPI{} };

    api.RegisterModule = [](const char* name, void* ref)
        {
            auto reflection{ static_cast<ComponentReflection*>(ref) };
            FlEntityComponentSystemKernel::Instance().RegisterModule(name, *reflection);
        };
    api.AddComponent = [](const char* name, uint32_t e)
        {
            return FlEntityComponentSystemKernel::Instance().AddComponent(name, e);
        };
    api.RemoveComponent = [](const char* name, uint32_t e)
        {
            FlEntityComponentSystemKernel::Instance().RemoveComponent(name, e);
        };
    api.GetComponent = [](const char* name, uint32_t e)
        {
            return FlEntityComponentSystemKernel::Instance().GetComponent(name, e);
        };
    api.HasComponent = [](const char* name, uint32_t e)
        {
            return FlEntityComponentSystemKernel::Instance().HasComponent(name, e);
        };
    api.ToLogInfo = [](const char* fmt, ...)
        {
            auto args{ va_list{} };
            va_start(args, fmt);
            auto size{ vsnprintf(nullptr, NULL, fmt, args) };
            va_end(args);

            if (size < NULL) return;

            auto result{ std::string(size, '\0') };
            va_start(args, fmt);
            vsnprintf(&result[Def::UIntZero], static_cast<size_t>(size) + Def::ULongLongOne, fmt, args);
            va_end(args);

            FlEntityComponentSystemKernel::Instance().ToLogInfo(result);
        };
    api.ToLogError = [](const char* fmt, ...)
        {
            auto args{ va_list{} };
            va_start(args, fmt);
            auto size{ vsnprintf(nullptr, NULL, fmt, args) };
            va_end(args);

            if (size < NULL) return;

            auto result{ std::string(size, '\0') };
            va_start(args, fmt);
            vsnprintf(&result[Def::UIntZero], static_cast<size_t>(size) + Def::ULongLongOne, fmt, args);
            va_end(args);

            FlEntityComponentSystemKernel::Instance().ToLogError(result);
        };

    return &api;
}

void FlScriptModuleLoader::Update() noexcept
{
    const auto now = std::chrono::steady_clock::now();

    for (auto it = m_pendingReloads.begin(); it != m_pendingReloads.end(); )
    {
        auto& pending = it->second;

        if (now - pending.lastModified > FlChronus::sec(Def::UIntOne))
        {
            ScanModule(pending.dllPath);  // ←ここで初めてホットリロード
            it = m_pendingReloads.erase(it);
        }
        else {
            ++it;
        }
    }
}

void FlScriptModuleLoader::ScanModule(const std::filesystem::path& dllPath) noexcept
{
    auto moduleName = dllPath.stem().string();
    moduleName = moduleName.substr(0, moduleName.find("_hotload"));

    for (auto& m : m_modules)
    {
        if (m.moduleName == moduleName)
        {
            // 更新検知 → ホットリロード
            HotReload(m);
            GetLastWriteTime(dllPath, m.lastWriteTime);
            return;
        }
    }

    ScriptModule m;
    m.moduleName = moduleName;
    m.folder = dllPath.parent_path();
    m.originalDllPath = dllPath; // originalDllPath に保存

    GetLastWriteTime(m.originalDllPath, m.lastWriteTime);
    m_modules.push_back(m);

    Load(m);
}

void FlScriptModuleLoader::DeleteModule(const std::filesystem::path& dllPath) noexcept
{
    auto moduleName = dllPath.stem().string();
    moduleName = moduleName.substr(0, moduleName.find("_hotload"));

    for (auto it = m_modules.begin(); it != m_modules.end(); ++it)
    {
        if (it->moduleName == moduleName)
        {
            Unload(*it);
            m_modules.erase(it);
            return;
        }
    }
}

void FlScriptModuleLoader::ScanModules() noexcept
{
    m_modules.clear();
    for (auto& dir : std::filesystem::directory_iterator(m_rootDir))
    {
        if (!dir.is_directory()) continue;
        auto name = dir.path().filename().string();
        auto dll = dir.path() / (name + ".dll");
        if (!std::filesystem::exists(dll)) continue;

        ScriptModule m;
        m.moduleName = name;
        m.folder = dir.path();
        m.originalDllPath = dll; // originalDllPath に保存

        GetLastWriteTime(m.originalDllPath, m.lastWriteTime);
        m_modules.push_back(m);
    }
}

void FlScriptModuleLoader::LoadAll() noexcept
{
    for (auto& m : m_modules)
        Load(m);
}

// ---------------------------------------------------------
// Load: コピーしてロードする
// ---------------------------------------------------------
void FlScriptModuleLoader::Load(ScriptModule& m) noexcept
{
    if (!std::filesystem::exists(m.originalDllPath))
        return;

    auto tempDir = std::filesystem::temp_directory_path() / "FlTemp";
    std::filesystem::create_directories(tempDir);

    // ユニークな名前を生成 (タイムスタンプなど)
    auto timestamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto tempFileName = m.originalDllPath.stem().string() + "_" + timestamp + m.originalDllPath.extension().string();
    auto shadowPath = tempDir / tempFileName;

    try {
        std::filesystem::copy_file(m.originalDllPath, shadowPath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (...) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to copy DLL: %s", m.originalDllPath.string().c_str());
        return;
    }

    auto originalPdb = m.originalDllPath;
    originalPdb.replace_extension(".pdb");
    if (std::filesystem::exists(originalPdb))
    {
        auto shadowPdb = shadowPath;
        shadowPdb.replace_extension(".pdb");
        try {
            std::filesystem::copy_file(originalPdb, shadowPdb, std::filesystem::copy_options::overwrite_existing);
        }
        catch (...) { 
            FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed copy to %s", originalPdb.string().c_str());
        }
    }

    // 4. コピーしたDLLをロード
    m.loadedDllPath = shadowPath; // コピー先のパスを記憶
    m.dll = LoadLibraryA(m.loadedDllPath.string().c_str());

    if (!m.dll) return;

    auto set{ reinterpret_cast<SetRuntimeAPIFn>(GetProcAddress(m.dll, "SetAPI")) };
    if (set)
    {
        auto r{ set(CreateRuntimeAPI(), FlEditorAdministrator::Instance().GetContext()) };
        
        switch (r.code)
        {
        case FlResult::Fl_OK:
            FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success Loaded %s", m.originalDllPath.filename().string().c_str());
            break;
        case FlResult::Fl_FAIL:
            FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed Loaded %s  Line %d", m.originalDllPath.filename().string().c_str(), r.line);
            break;
        case FlResult::Fl_THROW:
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Throw Loaded %s  Line %d", m.originalDllPath.filename().string().c_str(), r.line);
            break;
        default: break;
        }
    }
}

// ---------------------------------------------------------
// Unload: ライブラリ解放後に一時ファイルを削除
// ---------------------------------------------------------
void FlScriptModuleLoader::Unload(ScriptModule& m) noexcept
{
    if (m.dll)
    {
        FreeLibrary(m.dll);
        m.dll = nullptr;
    }

    // ロックが外れたので一時ファイルを削除する
    if (!m.loadedDllPath.empty() && std::filesystem::exists(m.loadedDllPath))
    {
        std::error_code ec;
        std::filesystem::remove(m.loadedDllPath, ec);

        // PDBも削除
        auto pdbPath = m.loadedDllPath;
        pdbPath.replace_extension(".pdb");
        if (std::filesystem::exists(pdbPath)) {
            std::filesystem::remove(pdbPath, ec);
        }

        m.loadedDllPath.clear();
    }
}

void FlScriptModuleLoader::UnloadAll() noexcept
{
    for (auto& m : m_modules)
        Unload(m);
}

void FlScriptModuleLoader::HotReload(ScriptModule& m) noexcept
{
    FlEditorAdministrator::Instance().GetLogger()->AddLog("HotReload: %s", m.moduleName.c_str());

    // シーンをシリアライズしてデータを保持
    auto temp{ FlEntityComponentSystemKernel::Instance().SerializeScene() };

    // 新しい DLL をシャドウコピーして先にロードする
    if (!std::filesystem::exists(m.originalDllPath)) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("HotReload failed: original DLL not found %s", m.originalDllPath.string().c_str());
        return;
    }

    auto tempDir = std::filesystem::temp_directory_path() / "FlTemp";
    std::filesystem::create_directories(tempDir);

    auto timestamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto tempFileName = m.originalDllPath.stem().string() + "_" + timestamp + m.originalDllPath.extension().string();
    auto newShadowPath = tempDir / tempFileName;

    try {
        std::filesystem::copy_file(m.originalDllPath, newShadowPath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (...) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("HotReload: Failed to copy DLL: %s", m.originalDllPath.string().c_str());
        return;
    }

    auto originalPdb = m.originalDllPath;
    originalPdb.replace_extension(".pdb");
    if (std::filesystem::exists(originalPdb))
    {
        auto shadowPdb = newShadowPath;
        shadowPdb.replace_extension(".pdb");
        try {
            std::filesystem::copy_file(originalPdb, shadowPdb, std::filesystem::copy_options::overwrite_existing);
        }
        catch (...) {
            FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("HotReload: Failed copy PDB to %s", originalPdb.string().c_str());
        }
    }

    HMODULE newDll = LoadLibraryA(newShadowPath.string().c_str());
    if (!newDll)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("HotReload: LoadLibrary failed for %s", newShadowPath.string().c_str());
        std::error_code ec;
        std::filesystem::remove(newShadowPath, ec);
        return;
    }

    auto setNew = reinterpret_cast<SetRuntimeAPIFn>(GetProcAddress(newDll, "SetAPI"));
    if (!setNew)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("HotReload: SetAPI not found in %s", newShadowPath.string().c_str());
        FreeLibrary(newDll);
        std::error_code ec;
        std::filesystem::remove(newShadowPath, ec);
        return;
    }

    auto r = setNew(CreateRuntimeAPI(), FlEditorAdministrator::Instance().GetContext());
    if (r.code != FlResult::Fl_OK)
    {
        // 登録に失敗したら新しい DLL をアンロードして終了
        if (r.code == FlResult::Fl_FAIL)
            FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("HotReload: Failed Loaded %s  Line %d", newShadowPath.filename().string().c_str(), r.line);
        else if (r.code == FlResult::Fl_THROW)
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("HotReload: Throw Loaded %s  Line %d", newShadowPath.filename().string().c_str(), r.line);

        FreeLibrary(newDll);
        std::error_code ec;
        std::filesystem::remove(newShadowPath, ec);

        return;
    }

    // ここで古い DLL をアンロードして、ScriptModule 情報を新しいものに差し替える。
    HMODULE oldDll = m.dll;
    auto oldLoadedPath = m.loadedDllPath;

    // 更新：ScriptModule のハンドルとパスを切り替え
    m.dll = newDll;
    m.loadedDllPath = newShadowPath;

    // 古い DLL をアンロード（既に newDll により登録が新しい関数へ切り替わっているはず）
    if (oldDll)
    {
        FreeLibrary(oldDll);
    }

    // 古いシャドウファイルと PDB を削除
    if (!oldLoadedPath.empty() && std::filesystem::exists(oldLoadedPath))
    {
        std::error_code ec;
        std::filesystem::remove(oldLoadedPath, ec);
        auto pdbPath = oldLoadedPath;
        pdbPath.replace_extension(".pdb");
        if (std::filesystem::exists(pdbPath)) {
            std::filesystem::remove(pdbPath, ec);
        }
    }

    FlEntityComponentSystemKernel::Instance().DeserializeScene(temp);
}

bool FlScriptModuleLoader::GetLastWriteTime(const std::filesystem::path& p, FILETIME& out) noexcept
{
    if (!std::filesystem::exists(p))
        return false;

    HANDLE h = CreateFileA(p.string().c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return false;

    GetFileTime(h, nullptr, nullptr, &out);
    CloseHandle(h);
    return true;
}
