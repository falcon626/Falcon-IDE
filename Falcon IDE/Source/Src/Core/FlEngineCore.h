//#include "Framework/ComponentSystem/EngineApi/FlEngineApi.h++"
//
//// ====== 簡易World/ECS（最小骨子） ======
//struct EntityRec {
//    EntityId id{};
//    bool hasTransform{ false };
//    TransformPod tr{ 1u, sizeof(TransformPod), {0,0,0}, 0, {0,0,0,1}, {1,1,1}, 0, {} };
//
//    bool hasModel{ false };
//    ModelRendererPod mr{ 1u, sizeof(ModelRendererPod), {0}, 1, 1 };
//
//    bool hasCollider{ false };
//    ColliderDescPod col{ 1u, sizeof(ColliderDescPod), CS_Sphere, {0,0,0}, 1.0f, {0,0,0}, 0, {0,1,0}, 2.0f, 0, 0 };
//
//    // Script: typeId -> raw bytes
//    std::unordered_map<uint32_t, std::vector<uint8_t>> scripts;
//};
//
//static std::unordered_map<uint64_t, EntityRec> g_entities; // id -> record
//
//// ScriptType/Systems レジストリは別ファイルに分けてもよい（ここに最小実装）
//struct ScriptType {
//    uint32_t id;
//    const ScriptComponentOps* ops;
//    uint32_t size, align;
//};
//struct ScriptSystemInstance {
//    const ScriptSystemVt* vt;
//    void* self;
//    uint32_t scriptTypeId;
//};
//static uint32_t g_nextScriptTypeId = 1000;
//static std::unordered_map<uint32_t, ScriptType> g_scriptTypes; // id->type
//static std::vector<ScriptSystemInstance> g_scriptSystems;
//
//// ====== ユーティリティ ======
//static Result Ok() { return { RES_OK,0 }; }
//static Result Err() { return { RES_Failed,0 }; }
//
//static Float4x4 MakeSRT(const Float3& s, const Quat& q, const Float3& t) {
//    // 非正確でOKな最小SRT→4x4（行列は単純な回転から）
//    float x = q.x, y = q.y, z = q.z, w = q.w;
//    float xx = x * x, yy = y * y, zz = z * z;
//    float xy = x * y, xz = x * z, yz = y * z;
//    float wx = w * x, wy = w * y, wz = w * z;
//
//    Float4x4 M{};
//    M.m[0] = (1 - 2 * (yy + zz)) * s.x; M.m[1] = (2 * (xy + wz)) * s.x;   M.m[2] = (2 * (xz - wy)) * s.x;   M.m[3] = 0;
//    M.m[4] = (2 * (xy - wz)) * s.y;   M.m[5] = (1 - 2 * (xx + zz)) * s.y; M.m[6] = (2 * (yz + wx)) * s.y;   M.m[7] = 0;
//    M.m[8] = (2 * (xz + wy)) * s.z;   M.m[9] = (2 * (yz - wx)) * s.z;   M.m[10] = (1 - 2 * (xx + yy)) * s.z; M.m[11] = 0;
//    M.m[12] = t.x;               M.m[13] = t.y;               M.m[14] = t.z;               M.m[15] = 1;
//    return M;
//}
//
//// ====== EngineCore API実装 ======
//namespace EngineCore {
//
//    // Component: Transform/Model/Collider
//    Result HasComponent(EntityId e, ComponentKind k, bool* out) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end()) { *out = false; return Ok(); }
//        if (k == CK_Transform) *out = it->second.hasTransform;
//        else if (k == CK_ModelRenderer) *out = it->second.hasModel;
//        else if (k == CK_Collider) *out = it->second.hasCollider;
//        else *out = false;
//        return Ok();
//    }
//
//    Result AttachComponent(EntityId e, ComponentKind k) {
//        auto& rec = g_entities[e.v]; rec.id = e;
//        if (k == CK_Transform) rec.hasTransform = true;
//        else if (k == CK_ModelRenderer) rec.hasModel = true;
//        else if (k == CK_Collider) rec.hasCollider = true;
//        else return Err();
//        return Ok();
//    }
//
//    Result RemoveComponent(EntityId e, ComponentKind k) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end()) return Ok();
//        if (k == CK_Transform) it->second.hasTransform = false;
//        else if (k == CK_ModelRenderer) it->second.hasModel = false;
//        else if (k == CK_Collider) it->second.hasCollider = false;
//        else return Err();
//        return Ok();
//    }
//
//    Result GetTransform(EntityId e, TransformPod* io) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end() || !it->second.hasTransform) return Err();
//        *io = it->second.tr; return Ok();
//    }
//
//    Result SetTransform(EntityId e, const TransformPod* in) {
//        auto& rec = g_entities[e.v]; rec.id = e; rec.hasTransform = true; rec.tr = *in;
//        rec.tr.world = MakeSRT(rec.tr.scale, rec.tr.rotation, rec.tr.position);
//        return Ok();
//    }
//
//    // Script管理（HasScript/AttachScript/GetScriptBytes/RemoveScript 実装）
//    Result HasScript(EntityId e, uint32_t typeId, bool* out) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end()) { *out = false; return Ok(); }
//        *out = it->second.scripts.find(typeId) != it->second.scripts.end();
//        return Ok();
//    }
//
//    Result AttachScript(EntityId e, uint32_t typeId) {
//        auto tpIt = g_scriptTypes.find(typeId); if (tpIt == g_scriptTypes.end()) return Err();
//        auto& rec = g_entities[e.v]; rec.id = e;
//        auto& buf = rec.scripts[typeId];
//        buf.resize(tpIt->second.size);
//        if (tpIt->second.ops && tpIt->second.ops->construct) tpIt->second.ops->construct(buf.data());
//        return Ok();
//    }
//
//    Result GetScriptBytes(EntityId e, uint32_t typeId, void** out) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end()) return Err();
//        auto jt = it->second.scripts.find(typeId); if (jt == it->second.scripts.end()) return Err();
//        *out = jt->second.data(); return Ok();
//    }
//
//    Result RemoveScript(EntityId e, uint32_t typeId) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end()) return Ok();
//        auto tpIt = g_scriptTypes.find(typeId); if (tpIt != g_scriptTypes.end()) {
//            auto jt = it->second.scripts.find(typeId); if (jt != it->second.scripts.end()) {
//                if (tpIt->second.ops && tpIt->second.ops->destruct) tpIt->second.ops->destruct(jt->second.data());
//            }
//        }
//        it->second.scripts.erase(typeId);
//        return Ok();
//    }
//
//    // Render/Collision（最小ダミー実装）
//    Result LoadModel(const char* path, ModelHandle* out) {
//        // 実際はResourceManagerへ委譲。ここではハッシュもどき
//        uint64_t h = 1469598103934665603ull; for (; *path; ++path) h = (h ^ uint8_t(*path)) * 1099511628211ull;
//        out->h = h; return Ok();
//    }
//
//    Result SetEntityModel(EntityId e, ModelHandle mh) {
//        auto& rec = g_entities[e.v]; rec.id = e; rec.hasModel = true; rec.mr.model = mh;
//        return Ok();
//    }
//
//    Result UpdateModelParams(EntityId e, const ModelRendererPod* in) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end() || !it->second.hasModel) return Err();
//        it->second.mr.visible = in->visible;
//        it->second.mr.castShadow = in->castShadow;
//        return Ok();
//    }
//
//    Result SetCollider(EntityId e, const ColliderDescPod* d, ColliderHandle* out) {
//        auto& rec = g_entities[e.v]; rec.id = e; rec.hasCollider = true; rec.col = *d; out->h = e.v; return Ok();
//    }
//
//    Result UpdateCollider(EntityId e, const ColliderDescPod* d) {
//        auto it = g_entities.find(e.v); if (it == g_entities.end() || !it->second.hasCollider) return Err();
//        it->second.col = *d; return Ok();
//    }
//
//    Result RaycastFirst(const Float3* origin, const Float3* dir, float maxDist,
//        EntityId* outHitEntity, Float3* outPoint, Float3* outNormal) {
//        // 最小ダミー：範囲内にTransformがある最初のEntityを命中とする
//        for (auto& [id, rec] : g_entities) {
//            if (!rec.hasTransform) continue;
//            float dx = rec.tr.position.x - origin->x;
//            float dy = rec.tr.position.y - origin->y;
//            float dz = rec.tr.position.z - origin->z;
//            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
//            if (dist <= maxDist) {
//                *outHitEntity = EntityId{ id };
//                *outPoint = rec.tr.position; *outNormal = { 0,1,0 };
//                return Ok();
//            }
//        }
//        return Err();
//    }
//
//    // ===== Script登録（EXE所有） =====
//    Result RegisterScriptComponent(const char* /*name*/, const ScriptComponentOps* ops, uint32_t* outId) {
//        if (!ops || !outId) return Err();
//        uint32_t id = g_nextScriptTypeId++;
//        g_scriptTypes.emplace(id, ScriptType{ id, ops, ops->size, ops->align });
//        *outId = id;
//        return Ok();
//    }
//
//    Result RegisterScriptSystem(const ScriptSystemVt* vt, void* self) {
//        if (!vt) return Err();
//        g_scriptSystems.push_back(ScriptSystemInstance{ vt, self, vt->scriptTypeId });
//        if (vt->on_register) {
//            // on_registerでEngineApiを渡したい場合は外側から注入（後述）
//        }
//        return Ok();
//    }
//
//    const std::vector<ScriptSystemInstance>& ScriptSystems() { return g_scriptSystems; }
//    const ScriptType* FindScriptType(uint32_t id) {
//        auto it = g_scriptTypes.find(id); return it == g_scriptTypes.end() ? nullptr : &it->second;
//    }
//
//    // ===== フェーズ実行（RunPhase） =====
//    enum class Phase { PrePhysics, Physics, Presentation };
//
//    void RunPhase(Phase p) {
//        switch (p) {
//        case Phase::PrePhysics:
//            // 親子関係があるならここで積み上げ。最小実装では自己SRT→worldを更新
//            for (auto& [id, rec] : g_entities) {
//                if (!rec.hasTransform) continue;
//                rec.tr.world = MakeSRT(rec.tr.scale, rec.tr.rotation, rec.tr.position);
//            }
//            break;
//        case Phase::Physics:
//            // 最小：何もしない（CollisionAPIを呼ぶのはDLL/EXEロジック側）
//            break;
//        case Phase::Presentation:
//            // 最小：可視のモデルを「描画」するダミー（実際はDX12発行）
//            for (auto& [id, rec] : g_entities) {
//                if (rec.hasModel && rec.mr.visible) {
//                    // ここでDX12描画を呼ぶ（省略）
//                    // DrawModel(rec.mr.model, rec.tr.world);
//                }
//            }
//            break;
//        }
//    }
//} // namespace EngineCore
//
//// ===== EngineApi 構築 =====
//static WorldAPI BuildWorldAPI() {
//    WorldAPI a{};
//    a.size = sizeof(WorldAPI);
//    a.version = 1u;
//    a.has_component = &EngineCore::HasComponent;
//    a.attach_component = &EngineCore::AttachComponent;
//    a.remove_component = &EngineCore::RemoveComponent;
//    a.get_transform = &EngineCore::GetTransform;
//    a.set_transform = &EngineCore::SetTransform;
//    a.has_script = &EngineCore::HasScript;
//    a.attach_script = &EngineCore::AttachScript;
//    a.get_script_bytes = &EngineCore::GetScriptBytes;
//    a.remove_script = &EngineCore::RemoveScript;
//    return a;
//}
//static RenderAPI BuildRenderAPI() {
//    RenderAPI a{};
//    a.size = sizeof(RenderAPI);
//    a.version = 1u;
//    a.load_model_from_path = &EngineCore::LoadModel;
//    a.set_entity_model = &EngineCore::SetEntityModel;
//    a.update_model_params = &EngineCore::UpdateModelParams;
//    return a;
//}
//static CollisionAPI BuildCollisionAPI() {
//    CollisionAPI a{};
//    a.size = sizeof(CollisionAPI);
//    a.version = 1u;
//    a.set_entity_collider = &EngineCore::SetCollider;
//    a.update_collider = &EngineCore::UpdateCollider;
//    a.raycast_first = &EngineCore::RaycastFirst;
//    return a;
//}
//static ScriptRegistryAPI BuildScriptAPI() {
//    ScriptRegistryAPI a{};
//    a.size = sizeof(ScriptRegistryAPI);
//    a.version = 1u;
//    a.register_component = &EngineCore::RegisterScriptComponent;
//    a.register_system = &EngineCore::RegisterScriptSystem;
//    return a;
//}
//
//EngineApi BuildEngineApi(uint32_t apiVersion, ImGuiContext* ctx) {
//    EngineApi api{};
//    api.hdr.version = apiVersion;
//    api.hdr.size = sizeof(EngineApi);
//    api.world = BuildWorldAPI();
//    api.render = BuildRenderAPI();
//    api.collision = BuildCollisionAPI();
//    api.script = BuildScriptAPI();
//
//    api.imgui.set_context = [](void* c) {
//        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(c));
//        };
//
//    // DLLに渡す前にセットしておく
//    if (ctx && api.imgui.set_context)
//        api.imgui.set_context(ctx);
//
//    return api;
//}
