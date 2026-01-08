#include "FlEntityComponentSystemKernel.h"
#include "../../Framework/Module/RuntimeModule/ResistCamera.h"
#include "../../Framework/Module/RuntimeModule/ResistTransform.h"
#include "../../Framework/Module/RuntimeModule/ResistModelRender.h"
#include "../../Framework/Module/RuntimeModule/ResistNameAndTag.h"
#include "../../Framework/Module/RuntimeModule/ResistCollision.h"

#include "../../Framework/Module/RuntimeModule/Transform.h"

template<typename FnType>
static HMODULE GetModuleFromStdFunction(const std::function<FnType>& f)
{
    if (!f) return nullptr;
    // std::function 内部が関数ポインタ（非キャプチャ）なら target で取り出せる
    auto fp{ f.template target<typename std::add_pointer<FnType>::type>() };
    if (!fp) return nullptr;
    auto addr{ reinterpret_cast<void*>(*fp) };
    if (!addr) return nullptr;

    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) return nullptr;
    return static_cast<HMODULE>(mbi.AllocationBase);
}

void FlEntityComponentSystemKernel::initialize()
{
    ResistCamera ca;
    ResistTransform rt;
    ResistModelRender mt;
    ResistNameAndTag nt;
    ResistCollision ct;
}

const entityId FlEntityComponentSystemKernel::CreateEntity()
{
    entityId id{ Def::UIntZero };

    if (!m_freeIds.empty())
    {
        auto it = m_freeIds.begin();
        id = *it;
        m_freeIds.erase(it);
    }
    else
    {
        id = m_nextId;
        ++m_nextId;

        if (m_nextId == UINT32_MAX)
        {
            //throw std::overflow_error("Entity ID counter overflowed!");
            FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Entity ID counter overflowed!");
        }
    }

    m_activeIds.insert(id);

	AddComponent("Name", id);
	AddComponent("Transform", id);

    return id;
}

const bool FlEntityComponentSystemKernel::CreateEntity(entityId specifiedId)
{
    if (m_activeIds.count(specifiedId)) return false; // 既にアクティブなIDとして使われている (衝突)

    m_freeIds.erase(specifiedId);
    m_activeIds.insert(specifiedId);

    m_nextId = std::max(m_nextId, specifiedId + Def::UIntOne);

	AddComponent("Name", specifiedId);
	AddComponent("Transform", specifiedId);

    return true;
}

const bool FlEntityComponentSystemKernel::ReleaseId(entityId id)
{
    if (m_activeIds.find(id) == m_activeIds.end()) return false; // 存在しないID、または既に解放済みのIDを解放しようとした

    m_activeIds.erase(id);

    m_freeIds.insert(id);
    return true;
}

void FlEntityComponentSystemKernel::DestroyEntity(entityId id)
{
    for (auto& [_, name, storage] : m_storages) {
        auto it = storage.components.find(id);
        if (it != storage.components.end())
        {
            if (storage.reflection.Destroy && it->second)
                storage.reflection.Destroy(it->second);
            storage.components.erase(it);
        }
    }
    ReleaseId(id);
}

void FlEntityComponentSystemKernel::AllDestroyEntities()
{
    for (auto& id : m_activeIds)
        DestroyEntity(id);
}

void FlEntityComponentSystemKernel::RegisterModule(const std::string& typeName, ComponentReflection refl, const priority prio)
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto it = FindStorageIterator(typeName);

    if (it != m_storages.end())
    {
        // 既存の場合は更新
        std::get<priority>(*it) = prio;           // Priority更新
        std::get<ComponentStorage>(*it).reflection = refl; // Reflection更新
    }
    else
    {
        // 新規追加
        ComponentStorage newStorage{};
        newStorage.reflection = refl;
        m_storages.emplace_back(prio, typeName, std::move(newStorage));
    }

    // Priority (昇順) でソート
    std::stable_sort(m_storages.begin(), m_storages.end(),
        [](const auto& a, const auto& b) {
            return std::get<priority>(a) < std::get<priority>(b);
        });
}

void* FlEntityComponentSystemKernel::AddComponent(const std::string& name, entityId entity)
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto it = FindStorageIterator(name);
    if (it == m_storages.end())
        return nullptr;

    auto& storage = std::get<ComponentStorage>(*it);
    if (!storage.reflection.Create)
        return nullptr;

    void* comp = storage.reflection.Create();
    storage.components[entity] = comp;
    return comp;
}

void FlEntityComponentSystemKernel::RemoveComponent(const std::string& name, entityId entity)
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto itStorage = FindStorageIterator(name);
    if (itStorage == m_storages.end()) return;

    auto& s = std::get<ComponentStorage>(*itStorage);

    auto it = s.components.find(entity);
    if (it != s.components.end())
    {
        if (s.reflection.Destroy && it->second)
            s.reflection.Destroy(it->second);

        s.components.erase(it);
    }
}

void* FlEntityComponentSystemKernel::GetComponent(const std::string& name, entityId entity)
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto itStorage = FindStorageIterator(name);
    if (itStorage == m_storages.end()) return nullptr;

    auto& s = std::get<ComponentStorage>(*itStorage);
    auto it = s.components.find(entity);
    if (it == s.components.end()) return nullptr;
    return it->second;
}

bool FlEntityComponentSystemKernel::HasComponent(const std::string& name, entityId entity) const
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto itStorage = FindStorageIterator(name);
    if (itStorage == m_storages.end()) return false;
    return std::get<ComponentStorage>(*itStorage).components.count(entity) != FALSE;
}

void FlEntityComponentSystemKernel::UpdateAll(float dt)
{
    struct Snap {
        std::string name{};
        entityId id{};
        void* comp{};
        std::function<void(void*, entityId, float)> updateFn{}; // コピーして保持
        HMODULE ownerModule{};
    };

    std::vector<Snap> snaps;
    snaps.reserve(Def::BitMaskPos8);

    { // スナップ取得（ロック中）
        std::lock_guard<std::mutex> lk(m_mu);
        for (auto& [_, name, storage] : m_storages)
        {
            if (!storage.reflection.Update) continue;

            for (auto& [id, comp] : storage.components)
            {
                Snap s;
                s.name        = name;
                s.id          = id;
                s.comp        = comp;
                s.updateFn    = storage.reflection.Update;
                s.ownerModule = GetModuleFromStdFunction<void(void*, entityId, float)>(s.updateFn);
                snaps.push_back(std::move(s));
            }
        }
    } // ロック解除

    // 各呼び出し前に該当モジュールのカウントを増やしてから呼び出し、終わったら減らす
    for (auto& s : snaps)
    {
        HMODULE mod = s.ownerModule;
        if (mod)
        {
            // インクリメント（マップを初期化する可能性があるのでロックして操作）
            {
                std::lock_guard<std::mutex> lk(m_moduleCallsMu);
                auto& atom = m_moduleActiveCalls[mod];
                // atomic default-construct -> 0
                atom.fetch_add(1, std::memory_order_acq_rel);
            }
        }

        // 実行（例外はキャッチしてログに出すのが無難）
        try {
            if (s.updateFn) s.updateFn(s.comp, s.id, dt);
        }
        catch (const std::exception& ex) {
            ToLogError(std::string{ "UpdateAll: exception in updateFn: " } + ex.what());
        }
        catch (...) {
            ToLogError("UpdateAll: unknown exception in updateFn");
        }

        if (mod)
        {
            // デクリメントして notify
            {
                std::lock_guard<std::mutex> lk(m_moduleCallsMu);
                auto it = m_moduleActiveCalls.find(mod);
                if (it != m_moduleActiveCalls.end())
                    it->second.fetch_sub(1, std::memory_order_acq_rel);
            }
            m_moduleCv.notify_all();
        }
    }
}

nlohmann::json FlEntityComponentSystemKernel::SerializeEntity(entityId id)
{
    nlohmann::json obj;
    for (auto& [_, name, storage] : m_storages) {
        if (storage.components.count(id))
        {
            nlohmann::json cjson;
            storage.reflection.Serialize(storage.components[id], cjson);
            obj[name] = cjson;
        }
    }
    return obj;
}

void FlEntityComponentSystemKernel::DeserializeEntity(entityId id, const nlohmann::json& src)
{
    for (auto& [_, name, storage] : m_storages) {
        if (!src.contains(name)) continue;

        void* comp = AddComponent(name, id);
        if (!comp) continue;
        storage.reflection.Deserialize(comp, src[name]);
    }
}

nlohmann::json FlEntityComponentSystemKernel::SerializeScene()
{
	auto scene{ nlohmann::json{} };
	for (auto id : m_activeIds)
		scene["Entities"][std::to_string(id)] = SerializeEntity(id);

	return scene;
}

void FlEntityComponentSystemKernel::DeserializeScene(const nlohmann::json& src)
{
	if (!src.contains("Entities")) return;
	for (auto& [idStr, compJson] : src["Entities"].items()) {
		entityId id{ std::stoul(idStr) };
		CreateEntity(id);
		DeserializeEntity(id, compJson);
	}
}

void FlEntityComponentSystemKernel::DeserializeScene(
	const nlohmann::json& src,
	std::unordered_map<entityId, entityId>* outRemap)
{
	if (!src.contains("Entities")) return;

	std::unordered_map<entityId, entityId> localRemap;

	for (auto& [oldIdStr, _] : src["Entities"].items())
	{
		entityId oldId = std::stoul(oldIdStr);
		entityId newId = CreateEntity();
		localRemap[oldId] = newId;
	}

	for (auto& [oldIdStr, compJson] : src["Entities"].items())
	{
		entityId oldId = std::stoul(oldIdStr);
		DeserializeEntity(localRemap[oldId], compJson);
	}

	for (auto& [oldId, newId] : localRemap)
	{
		if (auto tc = static_cast<TransformComponent*>(
			GetComponent("Transform", newId)))
		{
			if (tc->m_parent != UINT32_MAX)
				tc->m_parent = localRemap[tc->m_parent];

			for (auto& c : tc->m_children)
				c = localRemap[c];
		}
	}

	if (outRemap)
		*outRemap = std::move(localRemap);
}

void FlEntityComponentSystemKernel::ClearComponent(const std::string_view name)
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto itStorage = FindStorageIterator(name);
    if (itStorage == m_storages.end()) return;

    auto& s = std::get<ComponentStorage>(*itStorage);
    for (auto& [id, comp] : s.components) {
        if (s.reflection.Destroy && comp)
            s.reflection.Destroy(comp);
    }
    s.components.clear();
    m_storages.erase(itStorage);
}

void FlEntityComponentSystemKernel::RemoveAllComponentsByModule(HMODULE module)
{
    if (!module) return;

    // (1) ロックして対象の型名を収集して即座に破棄する（コンポーネント実体を Destroy）
    {
        std::lock_guard<std::mutex> lk(m_mu);

        for (auto it = m_storages.begin(); it != m_storages.end(); )
        {
            auto& typeName = std::get<std::string>(*it);
            auto& storage = std::get<ComponentStorage>(*it);

            bool belongs = false;

            if (storage.reflection.Update) {
                HMODULE m = GetModuleFromStdFunction<void(void*, entityId, float)>(storage.reflection.Update);
                if (m == module) belongs = true;
            }
            if (!belongs && storage.reflection.RenderEditor) {
                HMODULE m = GetModuleFromStdFunction<void(void*, entityId)>(storage.reflection.RenderEditor);
                if (m == module) belongs = true;
            }
            if (!belongs && storage.reflection.Create) {
                HMODULE m = GetModuleFromStdFunction<void* ()>(storage.reflection.Create);
                if (m == module) belongs = true;
            }
            if (!belongs && storage.reflection.Copy) {
                HMODULE m = GetModuleFromStdFunction<void* (void*)>(storage.reflection.Copy);
                if (m == module) belongs = true;
            }
            if (!belongs && storage.reflection.Destroy) {
                HMODULE m = GetModuleFromStdFunction<void(void*)>(storage.reflection.Destroy);
                if (m == module) belongs = true;
            }
            if (!belongs && storage.reflection.Serialize)
            {
                HMODULE m = GetModuleFromStdFunction<void(void*, nlohmann::json&)>(storage.reflection.Serialize);
                if (m == module) belongs = true;
            }
            if (!belongs && storage.reflection.Deserialize)
            {
                HMODULE m = GetModuleFromStdFunction<void(void*, const nlohmann::json&)>(storage.reflection.Deserialize);
                if (m == module) belongs = true;
            }

            if (belongs)
            {
                // Destroy components
                for (auto& [id, comp] : storage.components)
                {
                    if (storage.reflection.Destroy && comp)
                    {
                        try { storage.reflection.Destroy(comp); }
                        catch (...) {}
                    }
                }
                storage.components.clear();
                storage.reflection = ComponentReflection{};

                it = m_storages.erase(it);
            }
            else ++it;
        }
    }

    // (2) アクティブコールが終わるのを待つ（モジュールごとのカウントが 0 になるまで）
    {
        std::unique_lock<std::mutex> lk(m_moduleCallsMu);
        m_moduleCv.wait(lk, [&]() {
            auto it = m_moduleActiveCalls.find(module);
            if (it == m_moduleActiveCalls.end()) return true;
            return it->second.load(std::memory_order_acquire) == 0;
            });
        // オプション：カウントエントリを消す
        m_moduleActiveCalls.erase(module);
    }
}

std::vector<entityId> FlEntityComponentSystemKernel::GetAllEntityIds() const
{
    std::lock_guard<std::mutex> lk(m_mu);
    std::vector<entityId> out;
    out.reserve(m_activeIds.size());
    for (auto id : m_activeIds) out.push_back(id);
    return out;
}

std::vector<std::string> FlEntityComponentSystemKernel::GetRegisteredComponentTypes() const
{
    std::lock_guard<std::mutex> lk(m_mu);
    std::vector<std::string> out;
    out.reserve(m_storages.size());
    for (auto& t : m_storages) out.push_back(std::get<std::string>(t));
    return out;
}

std::vector<std::string> FlEntityComponentSystemKernel::GetEntityComponentTypes(entityId id) const
{
    std::lock_guard<std::mutex> lk(m_mu);
    std::vector<std::string> out;
    for (auto& [_, name, storage] : m_storages) {
        if (storage.components.count(id)) out.push_back(name);
    }
    return out;
}

bool FlEntityComponentSystemKernel::RenderComponentEditor(const std::string& typeName, entityId id) const
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto it = FindStorageIterator(typeName);
    if (it == m_storages.end()) return false;
    const auto& storage = std::get<ComponentStorage>(*it);
    if (!storage.reflection.RenderEditor) return false;
    auto compIt = storage.components.find(id);
    if (compIt == storage.components.end()) return false;

    storage.reflection.RenderEditor(compIt->second, id);
    return true;
}
