#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // 結果コード
    typedef int32_t Result;
    enum : Result {
        Result_OK = 0,
        Result_Fail = 1,
        Result_Incompatible = 2,
        Result_InvalidArg = 3,
        Result_OutOfMemory = 4
    };

    // 16バイト GUID (POD)
    struct ModuleGUID {
        uint64_t hi;
        uint64_t lo;
    };

    // エンティティ ID（POD）
    typedef uint32_t EntityId;
    static constexpr EntityId INVALID_ENTITY = 0xFFFFFFFFu;

    // 最小 POD 型 Vec3 / Transform
    struct Vec3 { float x, y, z; };
    //struct TransformComponent {
    //    Vec3 position;
    //    Vec3 rotation;
    //    Vec3 scale;
    //};

    // コンポーネント種別 ID（EXE 側で決める）
    typedef uint32_t ComponentTypeId;

    // API: EXE が DLL に渡す関数ポインタ群（すべて POD）
    typedef Result(*fn_RegisterComponent)(ComponentTypeId componentType, const char* name, size_t size);
    typedef Result(*fn_RegisterSystem)(const char* systemName, void(*systemUpdateFunc)(float dt, void* userCtx), void* userCtx);

    // メモリ管理 ABI（DLL と EXE 間の所有ルールを明示）
    typedef void* (*fn_Alloc)(size_t bytes);   // EXE が DLL に渡す場合、DLL はこの割当で取得したメモリを EXE に対して返却しない（DLL が解放するか、EXE が解放するかは契約）
    typedef void  (*fn_Free)(void* ptr);

    // EXE -> DLL に渡すホスト API 構造体
    struct ModuleHostAPI {
        uint32_t hostApiVersion; 
        ImGuiContext* imguiContext;
        fn_RegisterComponent RegisterComponent;
        fn_RegisterSystem RegisterSystem;
        fn_Alloc Alloc;
        fn_Free Free;
    };

    // DLL がエクスポートする必要がある関数名（C ABI、POD のみ）
    typedef Result(*fn_Module_OnLoad)(const ModuleHostAPI* host);
    typedef void   (*fn_Module_OnUnload)();
    typedef Result(*fn_Module_Update)(float dt);

    // DLL は必ず以下シンボルをエクスポートすること（extern "C"）
    //
    // Module_GetGUID -> ModuleGUID
    // Module_GetAPIVersion -> uint32_t
    // Module_OnLoad(const ModuleHostAPI*)
    // Module_OnUnload()
    // Module_Update(float)
    //
    // 例外は投げないこと（DLL 内では捕捉すること）

#ifdef __cplusplus
} // extern "C"
#endif
