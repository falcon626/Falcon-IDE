//#pragma once
//#include "FlEngineApi.h++"
//
//// グローバル API バージョン
//#ifndef FL_ENGINE_API_VERSION
//#define FL_ENGINE_API_VERSION 0x00010000u
//#endif
//
//// 失敗時にエラーコードを返すための簡易マクロ
//#define FL_API_REQUIRE(cond, errcode) do { if (!(cond)) { return (errcode); } } while(0)
//
//// DLL側: EngineApi ヘッダの基本検証（hdr のみ）
//// サブAPI（world/render/collision/imgui/script）に size/version は現在持たせていない前提。
//inline int FlValidateEngineApiHeader(const EngineApi* api) {
//    if (!api)                               return -1; // null
//    if (api->hdr.version != FL_ENGINE_API_VERSION) return -2; // version mismatch
//    if (api->hdr.size < sizeof(EngineApi))  return -3; // size mismatch
//    return 0;
//}
//
//// DLL側: 必須ポインタ最小検証（必要十分な範囲にとどめる）
//inline int FlValidateEngineApiPointers(const EngineApi* api) {
//    if (!api) return -10;
//    // world
//    if (!api->world.has_component || !api->world.attach_component ||
//        !api->world.remove_component || !api->world.get_transform ||
//        !api->world.set_transform || !api->world.has_script ||
//        !api->world.attach_script || !api->world.get_script_bytes ||
//        !api->world.remove_script) return -11;
//    // render
//    if (!api->render.load_model_from_path ||
//        !api->render.set_entity_model ||
//        !api->render.update_model_params) return -12;
//    // collision
//    if (!api->collision.set_entity_collider ||
//        !api->collision.update_collider ||
//        !api->collision.raycast_first) return -13;
//    // imgui
//    if (!api->imgui.set_context) return -14;
//    // script registry
//    if (!api->script.register_component || !api->script.register_system) return -15;
//    return 0;
//}
//
//// DLL側: まとめチェック（エラーコードは負値で返す）
//inline int FlValidateEngineApi(const EngineApi* api) {
//    int r = FlValidateEngineApiHeader(api);
//    if (r) return r;
//    r = FlValidateEngineApiPointers(api);
//    if (r) return r;
//    return 0;
//}
