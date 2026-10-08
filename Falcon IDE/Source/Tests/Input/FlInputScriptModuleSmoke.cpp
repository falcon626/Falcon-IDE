namespace nlohmann { class json; }
#include "../../Src/Framework/Module/FlRunTimeAndDLLsCommon.h++"

namespace
{
    FlRuntimeAPI* g_runtimeAPI{};

    [[nodiscard]] FlResult Failure(long line) noexcept
    {
        return { FlResult::Fl_FAIL, line };
    }
}

extern "C" __declspec(dllexport) FlResult __cdecl SetAPI(FlRuntimeAPI* api, void*)
{
    g_runtimeAPI = api;
    if (!g_runtimeAPI || !g_runtimeAPI->Input)
        return Failure(__LINE__);

    const auto& input = *g_runtimeAPI->Input;
    if (!input.IsCompatible())
        return Failure(__LINE__);
    if (!input.IsKeyDown(FlKey::A))
        return Failure(__LINE__);
    if (!input.IsKeyPressed(FlKey::B))
        return Failure(__LINE__);
    if (!input.IsKeyReleased(FlKey::C))
        return Failure(__LINE__);
    if (input.IsKeyDown(static_cast<FlKey>(0xFFFFFFFFu)))
        return Failure(__LINE__);
    if (!input.IsMouseButtonDown(FlMouseButton::Left))
        return Failure(__LINE__);
    if (!input.IsMouseButtonPressed(FlMouseButton::Middle))
        return Failure(__LINE__);
    if (!input.IsMouseButtonReleased(FlMouseButton::Right))
        return Failure(__LINE__);
    if (input.IsMouseButtonDown(static_cast<FlMouseButton>(0xFFFFFFFFu)))
        return Failure(__LINE__);

    const auto position = input.GetMousePosition();
    const auto delta = input.GetMouseDelta();
    if (position.x != 320 || position.y != 240)
        return Failure(__LINE__);
    if (delta.x != -3 || delta.y != 7)
        return Failure(__LINE__);
    if (input.GetMouseWheelDelta() != FlMouseWheelDetent)
        return Failure(__LINE__);

    return { FlResult::Fl_OK, 0 };
}
