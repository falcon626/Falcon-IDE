#include "FlEntityComponentSystemKernel.h"
#include <charconv>
#include "../../Framework/Module/RuntimeModule/ResistCamera.h"
#include "../../Framework/Module/RuntimeModule/ResistTransform.h"
#include "../../Framework/Module/RuntimeModule/ResistModelRender.h"
#include "../../Framework/Module/RuntimeModule/ResistNameAndTag.h"
#include "../../Framework/Module/RuntimeModule/ResistCollision.h"

#include "../../Framework/Module/RuntimeModule/Transform.h"

namespace
{
    auto ReadSceneEntities(const nlohmann::json& src)
    {
        std::vector<std::pair<entityId, const nlohmann::json*>> entities;
        if (src.is_null()) return entities; // Legacy empty scenes were written as null.
        if (!src.is_object()) throw std::invalid_argument("Scene must be an object");
        if (!src.contains("Entities")) return entities;
        if (!src["Entities"].is_object()) throw std::invalid_argument("Entities must be an object");
        std::unordered_set<entityId> seen;
        for (auto& [key, components] : src["Entities"].items())
        {
            entityId id{};
            const auto end = key.data() + key.size();
            const auto result = std::from_chars(key.data(), end, id);
            if (result.ec != std::errc{} || result.ptr != end || id == UINT32_MAX || !seen.insert(id).second)
                throw std::invalid_argument("Invalid or duplicate entity ID: " + key);
            if (!components.is_object()) throw std::invalid_argument("Entity components must be an object");
            entities.emplace_back(id, &components);
        }
        return entities;
    }
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
    entityId id{};
    {
        std::lock_guard<std::mutex> lk(m_mu);
        if (!m_freeIds.empty())
        {
            auto it = m_freeIds.begin();
            id = *it;
            m_freeIds.erase(it);
        }
        else
        {
            if (m_nextId == UINT32_MAX) throw std::overflow_error("Entity ID counter overflowed");
            id = m_nextId++;
        }
        m_activeIds.insert(id);
    }
    AddComponent("Name", id);
    AddComponent("Transform", id);
    return id;
}


const bool FlEntityComponentSystemKernel::CreateEntity(entityId specifiedId)
{
    {
        std::lock_guard<std::mutex> lk(m_mu);
        if (specifiedId == UINT32_MAX || m_activeIds.count(specifiedId)) return false;
        m_freeIds.erase(specifiedId);
        m_activeIds.insert(specifiedId);
        m_nextId = std::max(m_nextId, specifiedId + Def::UIntOne);
    }
    AddComponent("Name", specifiedId);
    AddComponent("Transform", specifiedId);
    return true;
}


const bool FlEntityComponentSystemKernel::ReleaseId(entityId id)
{
    std::lock_guard<std::mutex> lk(m_mu);
    if (!m_activeIds.erase(id)) return false;
    m_freeIds.insert(id);
    return true;
}


void FlEntityComponentSystemKernel::DestroyEntity(entityId id)
{
    std::vector<std::shared_ptr<void>> removed;
    {
        std::lock_guard<std::mutex> lk(m_mu);
        for (auto& [_, name, storage] : m_storages)
        {
            auto it = storage.components.find(id);
            if (it == storage.components.end()) continue;
            removed.push_back(std::move(it->second));
            storage.components.erase(it);
        }
        m_unresolvedComponents.erase(id);
        if (m_activeIds.erase(id)) m_freeIds.insert(id);
    }
    // Snapshot references defer destruction until the current callback finishes.
}


void FlEntityComponentSystemKernel::AllDestroyEntities()
{
    for (auto id : GetAllEntityIds()) DestroyEntity(id);
}


void FlEntityComponentSystemKernel::RegisterModule(
    const std::string& typeName,
    ComponentReflection refl,
    const priority prio,
    HMODULE owner)
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto it = std::find_if(m_storages.begin(), m_storages.end(),
        [&](const auto& storage) {
            return std::get<std::string>(storage) == typeName
                && std::get<ComponentStorage>(storage).owner == owner;
        });

    if (it != m_storages.end())
    {
        // 既存の場合は更新
        std::get<priority>(*it) = prio;           // Priority更新
        std::get<ComponentStorage>(*it).reflection = refl; // Reflection更新
        std::get<ComponentStorage>(*it).owner = owner;
    }
    else
    {
        // 新規追加
        ComponentStorage newStorage{};
        newStorage.reflection = refl;
        newStorage.owner = owner;
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
    ComponentReflection reflection{};
    HMODULE owner{};
    {
        std::lock_guard<std::mutex> lk(m_mu);
        auto it = FindStorageIterator(name);
        if (it == m_storages.end() || !m_activeIds.count(entity)) return nullptr;
        auto& storage = std::get<ComponentStorage>(*it);
        if (!storage.reflection.Create) return nullptr;
        auto existing = storage.components.find(entity);
        if (existing != storage.components.end()) return existing->second.get();
        // A placeholder prevents recursive Create/Start from creating the same component twice.
        storage.components.emplace(entity, nullptr);
        reflection = storage.reflection;
        owner = storage.owner;
        RetainModule(owner);
    }

    auto destroy = [this, reflection, owner](void* comp) noexcept {
        try { if (reflection.Destroy && comp) reflection.Destroy(comp); }
        catch (...) {}
        ReleaseModule(owner);
    };
    std::shared_ptr<void> component;
    bool createFinished = false;
    try
    {
        void* raw = reflection.Create();
        createFinished = true;
        if (raw) component = std::shared_ptr<void>(raw, destroy);
        else ReleaseModule(owner);
    }
    catch (...)
    {
        // shared_ptr invokes its deleter if allocation of its control block fails.
        if (!createFinished) ReleaseModule(owner);
        std::lock_guard<std::mutex> lk(m_mu);
        auto it = FindStorageIterator(name);
        if (it != m_storages.end() && std::get<ComponentStorage>(*it).owner == owner)
        {
            auto& components = std::get<ComponentStorage>(*it).components;
            auto pending = components.find(entity);
            if (pending != components.end() && !pending->second) components.erase(pending);
        }
        throw;
    }

    std::lock_guard<std::mutex> lk(m_mu);
    auto it = FindStorageIterator(name);
    if (it == m_storages.end() || std::get<ComponentStorage>(*it).owner != owner) return nullptr;
    auto& components = std::get<ComponentStorage>(*it).components;
    auto pending = components.find(entity);
    if (pending == components.end()) return nullptr;
    if (pending->second) return pending->second.get();
    if (!component) { components.erase(pending); return nullptr; }
    pending->second = component;
    return component.get();
}


void FlEntityComponentSystemKernel::RemoveComponent(const std::string& name, entityId entity)
{
    std::shared_ptr<void> removed;
    {
        std::lock_guard<std::mutex> lk(m_mu);
        auto unknown = m_unresolvedComponents.find(entity);
        if (unknown != m_unresolvedComponents.end()) unknown->second.erase(name);
        auto itStorage = FindStorageIterator(name);
        if (itStorage == m_storages.end()) return;
        auto& components = std::get<ComponentStorage>(*itStorage).components;
        auto it = components.find(entity);
        if (it == components.end()) return;
        removed = std::move(it->second);
        components.erase(it);
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
    return it->second.get();
}

bool FlEntityComponentSystemKernel::HasComponent(const std::string& name, entityId entity) const
{
    std::lock_guard<std::mutex> lk(m_mu);
    auto itStorage = FindStorageIterator(name);
    if (itStorage == m_storages.end()) return false;
    auto& components = std::get<ComponentStorage>(*itStorage).components;
    auto it = components.find(entity);
    return it != components.end() && it->second != nullptr;
}

void FlEntityComponentSystemKernel::UpdateAll(float dt)
{
    struct Snap {
        entityId id{};
        std::shared_ptr<void> comp;
        UpdateFn updateFn{};
    };
    std::vector<Snap> snaps;
    {
        std::lock_guard<std::mutex> lk(m_mu);
        for (auto& [_, name, storage] : m_storages)
        {
            if (!storage.reflection.Update) continue;
            for (auto& [id, comp] : storage.components)
                if (comp) snaps.push_back({ id, comp, storage.reflection.Update });
        }
    }
    // ponytail: removed components finish this snapshot; generation handles are needed for long-lived references.
    for (auto& snap : snaps)
    {
        try { snap.updateFn(snap.comp.get(), snap.id, dt); }
        catch (const std::exception& ex) { ToLogError(std::string{ "UpdateAll: " } + ex.what()); }
        catch (...) { ToLogError("UpdateAll: unknown exception"); }
    }
}


nlohmann::json FlEntityComponentSystemKernel::SerializeEntity(entityId id)
{
    struct Snap { std::string name; std::shared_ptr<void> comp; SerializeFn serialize; };
    std::vector<Snap> snaps;
    auto obj = nlohmann::json::object();
    {
        std::lock_guard<std::mutex> lk(m_mu);
        auto unknown = m_unresolvedComponents.find(id);
        if (unknown != m_unresolvedComponents.end()) obj = unknown->second;
        for (auto& [_, name, storage] : m_storages)
        {
            auto it = storage.components.find(id);
            if (it != storage.components.end() && it->second && storage.reflection.Serialize)
                snaps.push_back({ name, it->second, storage.reflection.Serialize });
        }
    }
    for (auto& snap : snaps)
    {
        nlohmann::json value;
        snap.serialize(snap.comp.get(), value);
        obj[snap.name] = std::move(value);
    }
    return obj;
}


void FlEntityComponentSystemKernel::DeserializeEntity(entityId id, const nlohmann::json& src)
{
    if (!src.is_object()) throw std::invalid_argument("Entity components must be an object");
    {
        std::lock_guard<std::mutex> lk(m_mu);
        m_unresolvedComponents[id] = src;
    }
    for (auto& [name, value] : src.items())
    {
        AddComponent(name, id);
        std::shared_ptr<void> component;
        DeserializeFn deserialize{};
        {
            std::lock_guard<std::mutex> lk(m_mu);
            auto it = FindStorageIterator(name);
            if (it == m_storages.end()) continue;
            auto& storage = std::get<ComponentStorage>(*it);
            auto comp = storage.components.find(id);
            if (comp == storage.components.end() || !comp->second || !storage.reflection.Deserialize) continue;
            component = comp->second;
            deserialize = storage.reflection.Deserialize;
        }
        deserialize(component.get(), value);
    }
}


nlohmann::json FlEntityComponentSystemKernel::SerializeScene()
{
    auto scene = nlohmann::json{ { "Entities", nlohmann::json::object() } };
    for (auto id : GetAllEntityIds()) scene["Entities"][std::to_string(id)] = SerializeEntity(id);
    return scene;
}


void FlEntityComponentSystemKernel::DeserializeScene(const nlohmann::json& src)
{
    const auto entities = ReadSceneEntities(src); // Validate the entire document before changing the scene.
    for (const auto& [id, components] : entities)
    {
        CreateEntity(id);
        DeserializeEntity(id, *components);
    }
}


void FlEntityComponentSystemKernel::DeserializeScene(
    const nlohmann::json& src,
    std::unordered_map<entityId, entityId>* outRemap)
{
    const auto entities = ReadSceneEntities(src);
    std::unordered_map<entityId, entityId> localRemap;
    for (const auto& [oldId, _] : entities) localRemap.emplace(oldId, CreateEntity());
    for (const auto& [oldId, components] : entities)
        DeserializeEntity(localRemap.at(oldId), *components);

    for (const auto& [oldId, newId] : localRemap)
    {
        auto* transform = static_cast<TransformComponent*>(GetComponent("Transform", newId));
        if (!transform) continue;
        if (transform->m_parent != UINT32_MAX)
        {
            auto parent = localRemap.find(transform->m_parent);
            transform->m_parent = parent == localRemap.end() ? UINT32_MAX : parent->second;
        }
        for (auto child = transform->m_children.begin(); child != transform->m_children.end(); )
        {
            auto mapped = localRemap.find(*child);
            if (mapped == localRemap.end()) child = transform->m_children.erase(child);
            else { *child = mapped->second; ++child; }
        }
    }
    if (outRemap) *outRemap = std::move(localRemap);
}


void FlEntityComponentSystemKernel::ClearComponent(const std::string_view name)
{
    ComponentStorage removed;
    {
        std::lock_guard<std::mutex> lk(m_mu);
        for (auto& [_, value] : m_unresolvedComponents) value.erase(std::string(name));
        auto it = FindStorageIterator(name);
        if (it == m_storages.end()) return;
        removed = std::move(std::get<ComponentStorage>(*it));
        m_storages.erase(it);
    }
}


void FlEntityComponentSystemKernel::RemoveAllComponentsByModule(HMODULE module)
{
    if (!module) return;
    std::vector<ComponentStorage> removed;
    {
        std::lock_guard<std::mutex> lk(m_mu);
        for (auto it = m_storages.begin(); it != m_storages.end(); )
        {
            auto& storage = std::get<ComponentStorage>(*it);
            if (storage.owner != module) { ++it; continue; }
            removed.push_back(std::move(storage));
            it = m_storages.erase(it);
        }
    }
    // Release our references before waiting for callbacks/snapshots held by other threads.
    removed.clear();
    std::unique_lock<std::mutex> lk(m_moduleCallsMu);
    m_moduleCv.wait(lk, [&]() {
        auto it = m_moduleActiveCalls.find(module);
        return it == m_moduleActiveCalls.end() || it->second.load(std::memory_order_acquire) == 0;
    });
    m_moduleActiveCalls.erase(module);
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
        auto it = storage.components.find(id);
        if (it != storage.components.end() && it->second) out.push_back(name);
    }
    return out;
}

bool FlEntityComponentSystemKernel::RenderComponentEditor(const std::string& typeName, entityId id) const
{
    std::shared_ptr<void> component;
    RenderEditorFn render{};
    {
        std::lock_guard<std::mutex> lk(m_mu);
        auto it = FindStorageIterator(typeName);
        if (it == m_storages.end()) return false;
        const auto& storage = std::get<ComponentStorage>(*it);
        auto comp = storage.components.find(id);
        if (comp == storage.components.end() || !comp->second || !storage.reflection.RenderEditor) return false;
        component = comp->second;
        render = storage.reflection.RenderEditor;
    }
    render(component.get(), id);
    return true;
}


void FlEntityComponentSystemKernel::RetainModule(HMODULE module)
{
    if (!module) return;
    std::lock_guard<std::mutex> lk(m_moduleCallsMu);
    m_moduleActiveCalls[module].fetch_add(1, std::memory_order_acq_rel);
}

void FlEntityComponentSystemKernel::ReleaseModule(HMODULE module) noexcept
{
    if (!module) return;
    {
        std::lock_guard<std::mutex> lk(m_moduleCallsMu);
        auto it = m_moduleActiveCalls.find(module);
        if (it != m_moduleActiveCalls.end()) it->second.fetch_sub(1, std::memory_order_acq_rel);
    }
    m_moduleCv.notify_all();
}
