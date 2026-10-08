namespace nlohmann { class json; }
#include "../../Src/Framework/Module/FlRunTimeAndDLLsCommon.h++"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>

static_assert(std::is_standard_layout_v<FlInputAPI>);
static_assert(std::is_trivially_copyable_v<FlInputAPI>);
static_assert(sizeof(FlInputVector2) == sizeof(std::int32_t) * 2);
#if defined(_WIN64)
static_assert(sizeof(FlInputAPI) == 96);
static_assert(offsetof(FlInputAPI, MouseWheelDelta) == 88);
#endif

struct LegacyRuntimeAPI
{
    std::uint32_t(*CreateEntity)();
    void(*DestroyEntity)(std::uint32_t entity);
    void* (*AddComponent)(const char* typeName, std::uint32_t entity);
    void(*RemoveComponent)(const char* typeName, std::uint32_t entity);
    void* (*GetComponent)(const char* typeName, std::uint32_t entity);
    bool(*HasComponent)(const char* typeName, std::uint32_t entity);
    void(*RegisterModule)(const char* typeName, void* reflection);
    void(*ToLogInfo)(const char* fmt, ...);
    void(*ToLogError)(const char* fmt, ...);
};

static_assert(offsetof(FlRuntimeAPI, Input) == sizeof(LegacyRuntimeAPI));
static_assert(sizeof(FlRuntimeAPI) == sizeof(LegacyRuntimeAPI) + sizeof(const FlInputAPI*));
#if defined(_WIN64)
static_assert(offsetof(FlRuntimeAPI, Input) == 72);
static_assert(sizeof(FlRuntimeAPI) == 80);
#endif

namespace
{
    std::uint8_t __cdecl KeyDown(std::uint32_t key) noexcept
    {
        return key == static_cast<std::uint32_t>(FlKey::A) ? 1u : 0u;
    }

    std::uint8_t __cdecl KeyPressed(std::uint32_t key) noexcept
    {
        return key == static_cast<std::uint32_t>(FlKey::B) ? 1u : 0u;
    }

    std::uint8_t __cdecl KeyReleased(std::uint32_t key) noexcept
    {
        return key == static_cast<std::uint32_t>(FlKey::C) ? 1u : 0u;
    }

    std::uint8_t __cdecl MouseDown(std::uint32_t button) noexcept
    {
        return button == static_cast<std::uint32_t>(FlMouseButton::Left) ? 1u : 0u;
    }

    std::uint8_t __cdecl MousePressed(std::uint32_t button) noexcept
    {
        return button == static_cast<std::uint32_t>(FlMouseButton::Middle) ? 1u : 0u;
    }

    std::uint8_t __cdecl MouseReleased(std::uint32_t button) noexcept
    {
        return button == static_cast<std::uint32_t>(FlMouseButton::Right) ? 1u : 0u;
    }

    std::int32_t __cdecl MouseX() noexcept { return 320; }
    std::int32_t __cdecl MouseY() noexcept { return 240; }
    std::int32_t __cdecl MouseDeltaX() noexcept { return -3; }
    std::int32_t __cdecl MouseDeltaY() noexcept { return 7; }
    std::int32_t __cdecl MouseWheelDelta() noexcept { return FlMouseWheelDetent; }

    int CheckWrapperGuards(const FlInputAPI& valid) noexcept
    {
        auto futureVersion = valid;
        ++futureVersion.version;
        if (!futureVersion.IsKeyDown(FlKey::A))
            return 1;

        auto wrongVersion = valid;
        wrongVersion.version = FlInputAPIVersion - 1;
        if (wrongVersion.IsKeyDown(FlKey::A))
            return 2;

        auto shortTable = valid;
        shortTable.size = static_cast<std::uint32_t>(offsetof(FlInputAPI, MouseWheelDelta));
        if (shortTable.IsKeyDown(FlKey::A))
            return 3;

        auto nullCallbacks = FlInputAPI{};
        nullCallbacks.size = static_cast<std::uint32_t>(sizeof(FlInputAPI));
        nullCallbacks.version = FlInputAPIVersion;
        if (nullCallbacks.IsKeyDown(FlKey::A)
            || nullCallbacks.IsKeyPressed(FlKey::B)
            || nullCallbacks.IsKeyReleased(FlKey::C)
            || nullCallbacks.IsMouseButtonDown(FlMouseButton::Left)
            || nullCallbacks.IsMouseButtonPressed(FlMouseButton::Middle)
            || nullCallbacks.IsMouseButtonReleased(FlMouseButton::Right))
            return 4;
        const auto nullPosition = nullCallbacks.GetMousePosition();
        const auto nullDelta = nullCallbacks.GetMouseDelta();
        if (nullPosition.x != 0 || nullPosition.y != 0
            || nullDelta.x != 0 || nullDelta.y != 0
            || nullCallbacks.GetMouseWheelDelta() != 0)
            return 5;
        return 0;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: FlInputScriptModuleHostSmoke <module.dll>\n");
        return 1;
    }

    const FlInputAPI input{
        static_cast<std::uint32_t>(sizeof(FlInputAPI)),
        FlInputAPIVersion,
        &KeyDown,
        &KeyPressed,
        &KeyReleased,
        &MouseDown,
        &MousePressed,
        &MouseReleased,
        &MouseX,
        &MouseY,
        &MouseDeltaX,
        &MouseDeltaY,
        &MouseWheelDelta,
    };

    if (const auto guardFailure = CheckWrapperGuards(input))
    {
        std::fprintf(stderr, "FAIL: wrapper guard %d\n", guardFailure);
        return 2;
    }

    const auto module = LoadLibraryA(argv[1]);
    if (!module)
    {
        std::fprintf(stderr, "FAIL: LoadLibrary error %lu\n", GetLastError());
        return 3;
    }

    const auto setAPI = reinterpret_cast<SetRuntimeAPIFn>(GetProcAddress(module, "SetAPI"));
    if (!setAPI)
    {
        std::fprintf(stderr, "FAIL: SetAPI export missing\n");
        FreeLibrary(module);
        return 4;
    }

    if (setAPI(nullptr, nullptr).code != FlResult::Fl_FAIL)
    {
        std::fprintf(stderr, "FAIL: ScriptModule accepted null runtime API\n");
        FreeLibrary(module);
        return 5;
    }

    auto runtime = FlRuntimeAPI{};
    if (setAPI(&runtime, nullptr).code != FlResult::Fl_FAIL)
    {
        std::fprintf(stderr, "FAIL: ScriptModule accepted null Input API\n");
        FreeLibrary(module);
        return 6;
    }

    auto shortInput = input;
    shortInput.size = static_cast<std::uint32_t>(offsetof(FlInputAPI, MouseWheelDelta));
    runtime.Input = &shortInput;
    if (setAPI(&runtime, nullptr).code != FlResult::Fl_FAIL)
    {
        std::fprintf(stderr, "FAIL: ScriptModule accepted short Input API\n");
        FreeLibrary(module);
        return 7;
    }

    auto oldInput = input;
    oldInput.version = FlInputAPIVersion - 1;
    runtime.Input = &oldInput;
    if (setAPI(&runtime, nullptr).code != FlResult::Fl_FAIL)
    {
        std::fprintf(stderr, "FAIL: ScriptModule accepted old Input API\n");
        FreeLibrary(module);
        return 8;
    }

    runtime.Input = &input;
    const auto result = setAPI(&runtime, nullptr);
    FreeLibrary(module);

    if (result.code != FlResult::Fl_OK)
    {
        std::fprintf(stderr, "FAIL: ScriptModule rejected API at line %ld\n", result.line);
        return 9;
    }

    std::puts("FlInput ScriptModule ABI smoke: PASS");
    return 0;
}
