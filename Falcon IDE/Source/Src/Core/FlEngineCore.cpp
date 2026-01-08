//#pragma once
//#include "FlEngineCore.h"
//
//static Result World_HasComponent(EntityId e, ComponentKind kind, bool* out) {
//    return EngineCore::HasComponent(e, kind, out);
//}
//
//static Result World_AttachComponent(EntityId e, ComponentKind kind) {
//    return EngineCore::AttachComponent(e, kind);
//}
//
//static Result World_RemoveComponent(EntityId e, ComponentKind kind) {
//    return EngineCore::RemoveComponent(e, kind);
//}
//
//static Result World_HasScript(EntityId e, uint32_t typeId, bool* out) {
//    return EngineCore::HasScript(e, typeId, out);
//}
//
//static Result World_AttachScript(EntityId e, uint32_t typeId) {
//    return EngineCore::AttachScript(e, typeId);
//}
//
//static Result World_GetScriptBytes(EntityId e, uint32_t typeId, void** outBytes) {
//    return EngineCore::GetScriptBytes(e, typeId, outBytes);
//}
//
//static Result World_RemoveScript(EntityId e, uint32_t typeId) {
//    return EngineCore::RemoveScript(e, typeId);
//}
//
//static Result Script_RegisterComponent(const char* name, const ScriptComponentOps* ops, uint32_t* outTypeId) {
//    return EngineCore::RegisterScriptComponent(name, ops, outTypeId);
//}
//
//static void SetImGuiContext(void* ctx) {
//    ImGui::SetCurrentContext((ImGuiContext*)ctx);
//}
//
//static Result Script_RegisterSystem(const ScriptSystemVt* vt, void* self) {
//    return EngineCore::RegisterScriptSystem(vt, self);
//}
//
//// ÉOÉçÅ[ÉoÉãEngineApi
//EngineApi g_engineApi = BuildEngineApi(0x00010000, FlEditorAdministrator::Instance().GetContext());