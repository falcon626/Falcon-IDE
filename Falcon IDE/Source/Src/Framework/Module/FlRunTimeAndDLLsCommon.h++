#pragma once
#include <cstdint>
#include <cstddef>

/// <summary>
/// Interface Boundary
/// </summary>

using entityId = uint32_t;
using priority = uint32_t;

using CreateFn       = void* (*)();
using DestroyFn      = void  (*)(void*);
using CopyFn         = void* (*)(void*);
using SerializeFn    = void  (*)(void*, nlohmann::json&);
using DeserializeFn  = void  (*)(void*, const nlohmann::json&);
using RenderEditorFn = void  (*)(void*, entityId);
using UpdateFn       = void  (*)(void*, entityId, float);

struct ComponentReflection
{
    CreateFn       Create       = nullptr;
    DestroyFn      Destroy      = nullptr;
    CopyFn         Copy         = nullptr;
    SerializeFn    Serialize    = nullptr;
    DeserializeFn  Deserialize  = nullptr;
    RenderEditorFn RenderEditor = nullptr;
    UpdateFn       Update       = nullptr;
};

#if __cplusplus
extern "C" 
#endif // __cplusplus
{
    // === C ABI Compatible Result Struct === 
    struct FlResult
    {
        enum : uint32_t {
            Fl_OK,
            Fl_FAIL,
            Fl_THROW
        }code;
        long line;
    };

    // === C ABI Compatible API Function Pointer Table === 
    struct FlRuntimeAPI
    {
        // --- ECS 基本 ---
        uint32_t(*CreateEntity)();
        void     (*DestroyEntity)(uint32_t entity);

        void* (*AddComponent)(const char* typeName, uint32_t entity);
        void  (*RemoveComponent)(const char* typeName, uint32_t entity);
        void* (*GetComponent)(const char* typeName, uint32_t entity);
        bool  (*HasComponent)(const char* typeName, uint32_t entity);

        // --- Reflection 登録 ---
        void (*RegisterModule)(const char* typeName, void* reflection);

        // --- ToLog ログ出力 ---
        void (*ToLogInfo) (const char* fmt, ...);
        void (*ToLogError)(const char* fmt, ...);
    };

    // --- DLL がエクスポートする関数 ---
    typedef FlResult (*SetRuntimeAPIFn)(FlRuntimeAPI* api, void* ctx);

} // extern "C"