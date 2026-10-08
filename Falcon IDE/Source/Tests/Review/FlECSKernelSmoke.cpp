#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Src/Framework/Resource/Json/json.hpp"
#include "FlECSRuntimeAPI.inc"

namespace Def { constexpr uint32_t UIntZero = 0, UIntOne = 1, BitMaskPos4 = 16; }
struct SmokeLogger {
    void AddLog(const std::string&) {}
    void AddErrorLog(const std::string&) {}
};
struct FlEditorAdministrator {
    static FlEditorAdministrator& Instance() { static FlEditorAdministrator instance; return instance; }
    SmokeLogger* GetLogger() { return &logger; }
    SmokeLogger logger;
};
struct ResistCamera { ResistCamera() {} };
struct ResistTransform { ResistTransform() {} };
struct ResistModelRender { ResistModelRender() {} };
struct ResistNameAndTag { ResistNameAndTag() {} };
struct ResistCollision { ResistCollision() {} };
struct TransformComponent { entityId m_parent = UINT32_MAX; std::vector<entityId> m_children; };

// The runner injects the current production header and implementation here.
// Only graphics initialization and the logger are adapted for this standalone check.
#include "FlECSKernelProduction.inc"

static auto& kernel = FlEntityComponentSystemKernel::Instance();
static int destroyed = 0;
static int updated = 0;
static entityId victim{};

static ComponentReflection Reflection()
{
    ComponentReflection reflection;
    reflection.Create = []() -> void* {
        kernel.GetComponent("Base", 10);
        kernel.ToLogInfo("Create reentered");
        return new int(7);
    };
    reflection.Destroy = [](void* value) {
        kernel.GetComponent("Base", 10);
        kernel.ToLogInfo("Destroy reentered");
        ++destroyed;
        delete static_cast<int*>(value);
    };
    reflection.Serialize = [](void* value, nlohmann::json& out) {
        kernel.GetComponent("Base", 10);
        out["value"] = *static_cast<int*>(value);
    };
    reflection.Deserialize = [](void* value, const nlohmann::json& in) {
        kernel.GetComponent("Base", 10);
        *static_cast<int*>(value) = in.at("value").get<int>();
    };
    reflection.RenderEditor = [](void* value, entityId id) {
        assert(kernel.GetComponent("Base", id));
        kernel.ToLogInfo("Inspector reentered");
        assert(*static_cast<int*>(value) == 7);
    };
    return reflection;
}

int main()
{
    kernel.RegisterModule("Base", Reflection());
    assert(kernel.CreateEntity(10));
    assert(kernel.AddComponent("Base", 10));
    assert(kernel.RenderComponentEditor("Base", 10));
    assert(kernel.SerializeEntity(10)["Base"]["value"] == 7);
    kernel.DeserializeEntity(10, { { "Base", { { "value", 9 } } }, { "MissingDll", { { "secret", 42 } } } });
    assert(kernel.SerializeScene()["Entities"]["10"]["MissingDll"]["secret"] == 42);
    assert(kernel.SerializeEntity(10)["Base"]["value"] == 9);
    kernel.RemoveComponent("MissingDll", 10);
    assert(!kernel.SerializeEntity(10).contains("MissingDll"));
    kernel.RemoveComponent("Base", 10);
    assert(destroyed == 1);

    // A later Update pointer must remain alive when an earlier Update removes it.
    victim = kernel.CreateEntity();
    auto remover = Reflection();
    remover.Update = [](void*, entityId, float) {
        kernel.RemoveComponent("Victim", victim);
        assert(destroyed == 1);
    };
    auto target = Reflection();
    target.Update = [](void* value, entityId, float) {
        assert(*static_cast<int*>(value) == 7);
        ++updated;
    };
    kernel.RegisterModule("Remover", remover, 0);
    kernel.RegisterModule("Victim", target, 1);
    kernel.AddComponent("Remover", 10);
    kernel.AddComponent("Victim", victim);
    kernel.UpdateAll(0.1f);
    assert(updated == 1 && destroyed == 2);
    kernel.ClearComponent("Remover");
    assert(destroyed == 3);

    // Unloading waits for the Inspector callback and its Destroy to finish.
    const auto owner = reinterpret_cast<HMODULE>(1);
    std::promise<void> entered, release;
    auto released = release.get_future().share();
    static std::promise<void>* enteredCallback;
    static std::shared_future<void>* releaseCallback;
    enteredCallback = &entered;
    releaseCallback = &released;
    auto owned = Reflection();
    owned.RenderEditor = [](void* value, entityId) {
        enteredCallback->set_value();
        releaseCallback->wait();
        assert(*static_cast<int*>(value) == 7);
    };
    kernel.RegisterModule("Owned", owned, 2, owner);
    kernel.AddComponent("Owned", 10);
    auto inspector = std::async(std::launch::async, [] { kernel.RenderComponentEditor("Owned", 10); });
    entered.get_future().wait();
    auto unload = std::async(std::launch::async, [owner] { kernel.RemoveAllComponentsByModule(owner); });
    assert(unload.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);
    release.set_value();
    inspector.get();
    unload.get();
    assert(destroyed == 4 && !kernel.HasComponent("Owned", 10));

    // Failed creation releases the owner pin and leaves no poisoned placeholder.
    auto failing = Reflection();
    failing.Create = []() -> void* { throw std::runtime_error("expected create failure"); };
    kernel.RegisterModule("Failing", failing, 2, owner);
    try { kernel.AddComponent("Failing", 10); assert(false); }
    catch (const std::runtime_error&) {}
    assert(!kernel.HasComponent("Failing", 10));
    kernel.RemoveAllComponentsByModule(owner);

    // Old/new registrations coexist; removing either owner preserves the other.
    const auto nextOwner = reinterpret_cast<HMODULE>(2);
    kernel.RegisterModule("Reload", Reflection(), 2, owner);
    kernel.AddComponent("Reload", 10);
    kernel.RegisterModule("Reload", Reflection(), 2, nextOwner);
    kernel.RemoveAllComponentsByModule(nextOwner); // Roll back a failed new DLL.
    assert(kernel.HasComponent("Reload", 10));
    kernel.RegisterModule("Reload", Reflection(), 2, nextOwner);
    kernel.RemoveAllComponentsByModule(owner);
    assert(kernel.AddComponent("Reload", 10));
    kernel.RemoveAllComponentsByModule(nextOwner);
    assert(destroyed == 6);
    kernel.AllDestroyEntities();
    assert(kernel.GetActiveIdCount() == 0);

    // Validate every key before either loading mode can mutate live entities.
    for (const auto& key : { "-1", "1suffix", "4294967296", "4294967295", "" })
    {
        auto invalid = nlohmann::json{ { "Entities", { { "0", nlohmann::json::object() }, { key, nlohmann::json::object() } } } };
        for (bool prefab : { false, true })
        {
            try {
                if (prefab) kernel.DeserializeScene(invalid, nullptr);
                else kernel.DeserializeScene(invalid);
                assert(false);
            }
            catch (const std::invalid_argument&) {}
            assert(kernel.GetActiveIdCount() == 0);
        }
    }
    auto duplicate = nlohmann::json{ { "Entities", { { "01", nlohmann::json::object() }, { "1", nlohmann::json::object() } } } };
    try { kernel.DeserializeScene(duplicate); assert(false); }
    catch (const std::invalid_argument&) {}
    assert(kernel.GetActiveIdCount() == 0);

    // Entity 0 and UINT32_MAX root references are valid; external prefab links are not.
    ComponentReflection transform;
    transform.Create = []() -> void* { return new TransformComponent; };
    transform.Destroy = [](void* value) { delete static_cast<TransformComponent*>(value); };
    transform.Deserialize = [](void* value, const nlohmann::json& in) {
        auto* component = static_cast<TransformComponent*>(value);
        component->m_parent = in.at("parent").get<entityId>();
        component->m_children = in.at("children").get<std::vector<entityId>>();
    };
    kernel.RegisterModule("Transform", transform);
    const auto scene = nlohmann::json{ { "Entities", {
        { "0", { { "Transform", { { "parent", UINT32_MAX }, { "children", { 1, 999 } } } } } },
        { "1", { { "Transform", { { "parent", 0 }, { "children", nlohmann::json::array() } } } } },
        { "2", { { "Transform", { { "parent", 999 }, { "children", nlohmann::json::array() } } } } }
    } } };
    kernel.DeserializeScene(scene);
    assert(kernel.IsActive(0));
    assert(static_cast<TransformComponent*>(kernel.GetComponent("Transform", 1))->m_parent == 0);
    kernel.AllDestroyEntities();
    std::unordered_map<entityId, entityId> remap;
    kernel.DeserializeScene(scene, &remap);
    const auto* root = static_cast<TransformComponent*>(kernel.GetComponent("Transform", remap.at(0)));
    const auto* child = static_cast<TransformComponent*>(kernel.GetComponent("Transform", remap.at(1)));
    const auto* external = static_cast<TransformComponent*>(kernel.GetComponent("Transform", remap.at(2)));
    assert(root->m_parent == UINT32_MAX && root->m_children == std::vector<entityId>{ remap.at(1) });
    assert(child->m_parent == remap.at(0));
    assert(external->m_parent == UINT32_MAX);
    kernel.AllDestroyEntities();
    return 0;
}
