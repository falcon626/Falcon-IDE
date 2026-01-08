#include "FlRuntimeRegistry.h"

// 内部キー
struct RegKey {
    std::string dll;
    uint32_t entityId;
    std::string comp;
    bool operator==(RegKey const& o) const noexcept {
        return dll == o.dll && entityId == o.entityId && comp == o.comp;
    }
};

// <functionoid:functor> Custom Hash for RegKey
struct RegKeyHash {
    std::size_t operator()(RegKey const& k) const noexcept {
        std::size_t h1 = std::hash<std::string>{}(k.dll);
        std::size_t h2 = std::hash<uint32_t>{}(k.entityId);
        std::size_t h3 = std::hash<std::string>{}(k.comp);

        std::size_t seed = h1;
        seed ^= h2 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct ComponentFuncs {
    using CreateFn = void* (*)(uint32_t);
    using DeleteFn = void(*)(uint32_t, void*);
    CreateFn create = nullptr;
    DeleteFn del = nullptr;
};

struct SystemEntry {
    HMODULE dll;
    SystemLogics* funcs;   // DLLから取得した関数ポインタ群
};

// マップ: RegKey -> void* (ポインタは DLL 管理)
static std::unordered_map<std::string, ComponentFuncs> g_loadedDllFuncs;
static std::unordered_map<RegKey, void*, RegKeyHash> g_registry;
static std::mutex g_registryMutex;

static std::unordered_map<std::string, SystemEntry> g_systems;

// ヘルパー
static RegKey MakeKey(const char* dllName, uint32_t entityId, const char* compName) {
    return RegKey{ dllName ? dllName : "", entityId, compName ? compName : "" };
}

extern "C" {

    int Runtime_RegisterComponent(const char* dllName, uint32_t entityId, const char* componentName, void* ptr) {
        if (!dllName || !componentName || !ptr) return -1;
        std::lock_guard<std::mutex> lk(g_registryMutex);
        RegKey k = MakeKey(dllName, entityId, componentName);
        // 上書き禁止（既に登録済みならエラーにする方針。必要であれば上書きを許可）
        if (g_registry.find(k) != g_registry.end()) return -2;
        g_registry.emplace(std::move(k), ptr);
        return 0;
    }

    int Runtime_UnregisterComponent(const char* dllName, uint32_t entityId, const char* componentName) {
        if (!dllName || !componentName) return -1;
        std::lock_guard<std::mutex> lk(g_registryMutex);
        RegKey k = MakeKey(dllName, entityId, componentName);
        auto it = g_registry.find(k);
        if (it == g_registry.end()) return -2;
        g_registry.erase(it);
        return 0;
    }

    void* Runtime_GetComponent(const char* dllName, uint32_t entityId, const char* componentName) {
        if (!dllName || !componentName) return nullptr;
        std::lock_guard<std::mutex> lk(g_registryMutex);
        RegKey k = MakeKey(dllName, entityId, componentName);
        auto it = g_registry.find(k);
        if (it == g_registry.end()) return nullptr;
        return it->second;
    }

    int Runtime_UnregisterAllForDll(const char* dllName) {
        if (!dllName) return -1;
        std::lock_guard<std::mutex> lk(g_registryMutex);
        std::vector<RegKey> toErase;
        toErase.reserve(32);
        for (auto& p : g_registry) {
            if (p.first.dll == dllName) toErase.push_back(p.first);
        }
        for (auto& k : toErase) g_registry.erase(k);
        return 0;
    }

    int Runtime_EnumerateComponentsForEntity(uint32_t entityId, RuntimeEnumCallback cb, void* user) {
        if (!cb) return -1;
        std::lock_guard<std::mutex> lk(g_registryMutex);
        for (auto& p : g_registry) {
            if (p.first.entityId == entityId) {
                cb(p.first.dll.c_str(), p.first.comp.c_str(), p.second, user);
            }
        }
        return 0;
    }

    bool Runtime_LoadDll(const char* dllName, const char* dllPath) {
        HMODULE mod = LoadLibraryA(dllPath);
        if (!mod) return false;

        // -------- Component --------
        ComponentFuncs f;
        f.create = (ComponentFuncs::CreateFn)GetProcAddress(mod, "Create_Component");
        f.del = (ComponentFuncs::DeleteFn)GetProcAddress(mod, "Delete_Component");

        if (!f.create || !f.del) {
            FreeLibrary(mod);
            return false;
        }
        g_loadedDllFuncs[dllName] = f;

        // -------- System --------
        const auto GetLogics{ (SystemLogics*(*)())GetProcAddress(mod, "Get_SystemLogics") };

        if (!GetLogics) {
            FreeLibrary(mod);
            return false;
        }
        SystemLogics* logics = GetLogics();

        // 登録
        SystemEntry entry{};
        entry.dll          = mod;
        entry.funcs        = logics;

        g_systems[dllName] = entry;

        return true;
    }


    // 追加: runtimeから作成
    void* Runtime_CreateComponent(const char* dllName, const char* componentName, uint32_t entityId) {
        if (!dllName || !componentName) return nullptr;

        auto it = g_loadedDllFuncs.find(dllName);
        if (it == g_loadedDllFuncs.end()) return nullptr;

        auto& f = it->second;
        if (!f.create) return nullptr;

        void* ptr = f.create(entityId);
        if (!ptr) return nullptr;

        Runtime_RegisterComponent(dllName, entityId, componentName, ptr);
        return ptr;
    }

    // 追加: runtimeから削除
    int Runtime_DeleteComponent(const char* dllName, const char* componentName, uint32_t entityId) {
        if (!dllName || !componentName) return -1;

        auto it = g_loadedDllFuncs.find(dllName);
        if (it == g_loadedDllFuncs.end()) return -2;

        void* ptr = Runtime_GetComponent(dllName, entityId, componentName);
        if (!ptr) return -3;

        // registry から外す
        Runtime_UnregisterComponent(dllName, entityId, componentName);

        auto& f = it->second;
        if (!f.del) return -4;

        f.del(entityId, ptr);
        return 0;
    }

    int Runtime_RegisterSystem(const char* dllName, void* instance, SystemLogics* funcs)
    {
        if (!dllName || !instance || !funcs) return -1;

        std::lock_guard<std::mutex> lk(g_registryMutex);

        auto it = g_systems.find(dllName);
        if (it != g_systems.end()) {
            // 二重登録防止（必要なら上書き許可も可能）
            return -2;
        }

        SystemEntry entry{};
        entry.dll = nullptr;     // LoadDll で埋まる
        entry.funcs = funcs;

        g_systems[dllName] = entry;
        return 0;
    }

    int Runtime_UnregisterSystem(const char* dllName)
    {
        if (!dllName) return -1;

        std::lock_guard<std::mutex> lk(g_registryMutex);

        auto it = g_systems.find(dllName);
        if (it == g_systems.end()) return -2;

        SystemEntry& entry = it->second;

        // DLL ハンドルもあれば解放
        if (entry.dll) {
            FreeLibrary(entry.dll);
        }

        g_systems.erase(it);
        return 0;
    }

    SystemLogics* Runtime_GetSystem(const char* dllName)
    {
        if (!dllName) return nullptr;

        std::lock_guard<std::mutex> lk(g_registryMutex);

        auto it = g_systems.find(dllName);
        if (it == g_systems.end()) return nullptr;

        return it->second.funcs;
    }

} // extern "C"
