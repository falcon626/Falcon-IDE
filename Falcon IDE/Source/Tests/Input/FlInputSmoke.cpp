#include "../../Src/Framework/System/Input/FlInput.h"

#include <Windows.h>

#include <cstdio>

namespace
{
    int g_failures{};

    void Check(bool condition, const char* message) noexcept
    {
        if (condition)
            return;

        std::fprintf(stderr, "FAIL: %s\n", message);
        ++g_failures;
    }

    LRESULT CALLBACK InputSmokeWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        FlInput::Instance().ProcessMessage(
            message, static_cast<std::uintptr_t>(wParam), static_cast<std::intptr_t>(lParam));
        return DefWindowProcW(window, message, wParam, lParam);
    }

    void BeginFrame(const char* message)
    {
        Check(FlInput::Instance().BeginFrame(), message);
    }

    void DrainMessages()
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    void SendMouseButton(HWND window, FlMouseButton button, bool down, int x = 10, int y = 20)
    {
        UINT message{};
        WPARAM wParam{};

        switch (button)
        {
        case FlMouseButton::Left:
            message = down ? WM_LBUTTONDOWN : WM_LBUTTONUP;
            wParam = down ? MK_LBUTTON : 0;
            break;
        case FlMouseButton::Middle:
            message = down ? WM_MBUTTONDOWN : WM_MBUTTONUP;
            wParam = down ? MK_MBUTTON : 0;
            break;
        case FlMouseButton::Right:
            message = down ? WM_RBUTTONDOWN : WM_RBUTTONUP;
            wParam = down ? MK_RBUTTON : 0;
            break;
        case FlMouseButton::X1:
            message = down ? WM_XBUTTONDOWN : WM_XBUTTONUP;
            wParam = MAKEWPARAM(down ? MK_XBUTTON1 : 0, XBUTTON1);
            break;
        case FlMouseButton::X2:
            message = down ? WM_XBUTTONDOWN : WM_XBUTTONUP;
            wParam = MAKEWPARAM(down ? MK_XBUTTON2 : 0, XBUTTON2);
            break;
        default:
            return;
        }

        SendMessageW(window, message, wParam, MAKELPARAM(x, y));
    }
}

int main()
{
    constexpr wchar_t WindowClassName[] = L"FalconInputSmokeWindow";
    const auto instance = GetModuleHandleW(nullptr);
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = InputSmokeWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = WindowClassName;

    Check(RegisterClassW(&windowClass) != 0, "register hidden test window class");
    const auto window = CreateWindowExW(
        0, WindowClassName, L"", WS_OVERLAPPED, 0, 0, 64, 64,
        nullptr, nullptr, instance, nullptr);
    Check(window != nullptr, "create hidden test window");

    if (!window || !FlInput::Instance().Initialize(window))
    {
        Check(false, "initialize FlInput");
        if (window) DestroyWindow(window);
        UnregisterClassW(WindowClassName, instance);
        return g_failures ? g_failures : 1;
    }

    SendMessageW(window, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), MAKELPARAM(0, 0));
    BeginFrame("first wheel frame");
    Check(!FlInput::Instance().IsKeyDown(FlKey::A), "A starts up");
    Check(FlInput::Instance().GetMouseWheelDelta() == FlMouseWheelDetent,
        "wheel input before the first frame is preserved");
    FlInput::Instance().EndFrame();

    BeginFrame("baseline frame");
    Check(FlInput::Instance().GetMouseWheelDelta() == 0, "wheel delta clears after first frame");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_MOUSEMOVE, 0, MAKELPARAM(100, 100));
    BeginFrame("first absolute mouse position");
    Check(FlInput::Instance().GetMouseDelta().x == 0 && FlInput::Instance().GetMouseDelta().y == 0,
        "first absolute mouse position after initialization is a baseline");
    FlInput::Instance().EndFrame();
    SendMessageW(window, WM_MOUSEMOVE, 0, MAKELPARAM(105, 105));
    BeginFrame("second absolute mouse position");
    Check(FlInput::Instance().GetMouseDelta().x == 5 && FlInput::Instance().GetMouseDelta().y == 5,
        "mouse delta starts after the initialization baseline");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_KEYDOWN, 'A', 1);
    BeginFrame("A press frame");
    Check(FlInput::Instance().IsKeyDown(FlKey::A), "A is down after keydown");
    Check(FlInput::Instance().IsKeyPressed(FlKey::A), "A pressed is one-frame true");
    Check(!FlInput::Instance().IsKeyReleased(FlKey::A), "A is not released on keydown");
    FlInput::Instance().EndFrame();

    BeginFrame("A held frame");
    Check(FlInput::Instance().IsKeyDown(FlKey::A), "A remains down while held");
    Check(!FlInput::Instance().IsKeyPressed(FlKey::A), "A pressed clears next frame");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_KEYUP, 'A', 0xC0000001);
    BeginFrame("A release frame");
    Check(!FlInput::Instance().IsKeyDown(FlKey::A), "A is up after keyup");
    Check(FlInput::Instance().IsKeyReleased(FlKey::A), "A released is one-frame true");
    FlInput::Instance().EndFrame();

    Check(PostMessageW(window, WM_KEYDOWN, 'B', 1) != FALSE, "post quick B keydown");
    Check(PostMessageW(window, WM_KEYUP, 'B', 0xC0000001) != FALSE, "post quick B keyup");
    DrainMessages();
    BeginFrame("same-pump B tap frame");
    Check(!FlInput::Instance().IsKeyDown(FlKey::B), "quick B tap ends up");
    Check(FlInput::Instance().IsKeyPressed(FlKey::B), "quick B tap keeps pressed transition");
    Check(FlInput::Instance().IsKeyReleased(FlKey::B), "quick B tap keeps released transition");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_KEYDOWN, 'C', 1);
    BeginFrame("C press before focus loss");
    Check(FlInput::Instance().IsKeyDown(FlKey::C), "C is down before focus loss");
    FlInput::Instance().EndFrame();
    SendMessageW(window, WM_ACTIVATEAPP, FALSE, 0);
    BeginFrame("focus loss frame");
    Check(!FlInput::Instance().IsKeyDown(FlKey::C), "focus loss clears held key");
    Check(FlInput::Instance().IsKeyReleased(FlKey::C), "focus loss emits release");
    FlInput::Instance().EndFrame();
    SendMessageW(window, WM_ACTIVATEAPP, TRUE, 0);

    SendMessageW(window, WM_MOUSEMOVE, 0, MAKELPARAM(100, 100));
    BeginFrame("mouse position before window focus loss");
    FlInput::Instance().EndFrame();

    SendMouseButton(window, FlMouseButton::Left, true, 100, 100);
    BeginFrame("mouse held before window focus loss");
    FlInput::Instance().EndFrame();
    SendMessageW(window, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    BeginFrame("window focus loss frame");
    Check(!FlInput::Instance().IsMouseButtonDown(FlMouseButton::Left),
        "window focus loss clears held mouse button");
    Check(FlInput::Instance().IsMouseButtonReleased(FlMouseButton::Left),
        "window focus loss emits mouse release");
    Check(FlInput::Instance().GetMouseDelta().x == 0 && FlInput::Instance().GetMouseDelta().y == 0,
        "window focus loss does not emit a fake mouse delta");
    Check(FlInput::Instance().GetMousePosition().x == 100
        && FlInput::Instance().GetMousePosition().y == 100,
        "window focus loss preserves the last valid mouse position");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);
    BeginFrame("focus returned without mouse movement");
    Check(FlInput::Instance().GetMouseDelta().x == 0 && FlInput::Instance().GetMouseDelta().y == 0,
        "focus return without a position keeps waiting for a baseline");
    Check(FlInput::Instance().GetMousePosition().x == 100
        && FlInput::Instance().GetMousePosition().y == 100,
        "focus return without movement preserves the last valid position");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(105, 105));
    BeginFrame("focus reacquired by mouse click");
    Check(FlInput::Instance().GetMouseDelta().x == 0 && FlInput::Instance().GetMouseDelta().y == 0,
        "focus-reacquiring click position becomes the baseline");
    Check(FlInput::Instance().IsMouseButtonPressed(FlMouseButton::Left),
        "focus-reacquiring click remains visible as a press");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_LBUTTONUP, 0, MAKELPARAM(105, 105));
    BeginFrame("focus-reacquiring click release");
    Check(FlInput::Instance().GetMouseDelta().x == 0 && FlInput::Instance().GetMouseDelta().y == 0,
        "click release at the baseline has zero delta");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_MOUSEMOVE, 0, MAKELPARAM(109, 112));
    BeginFrame("mouse delta frame");
    const auto position = FlInput::Instance().GetMousePosition();
    const auto delta = FlInput::Instance().GetMouseDelta();
    Check(position.x == 109 && position.y == 112, "mouse position uses client coordinates");
    Check(delta.x == 4 && delta.y == 7, "mouse delta is frame-local");
    FlInput::Instance().EndFrame();

    constexpr FlMouseButton Buttons[] = {
        FlMouseButton::Left,
        FlMouseButton::Middle,
        FlMouseButton::Right,
        FlMouseButton::X1,
        FlMouseButton::X2,
    };
    for (const auto button : Buttons)
    {
        SendMouseButton(window, button, true);
        BeginFrame("mouse button press frame");
        Check(FlInput::Instance().IsMouseButtonDown(button), "mouse button down");
        Check(FlInput::Instance().IsMouseButtonPressed(button), "mouse button pressed");
        FlInput::Instance().EndFrame();

        SendMouseButton(window, button, false);
        BeginFrame("mouse button release frame");
        Check(!FlInput::Instance().IsMouseButtonDown(button), "mouse button up");
        Check(FlInput::Instance().IsMouseButtonReleased(button), "mouse button released");
        FlInput::Instance().EndFrame();
    }

    Check(PostMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(10, 20)) != FALSE,
        "post quick mouse down");
    Check(PostMessageW(window, WM_LBUTTONUP, 0, MAKELPARAM(10, 20)) != FALSE,
        "post quick mouse up");
    DrainMessages();
    BeginFrame("same-pump mouse tap frame");
    Check(FlInput::Instance().IsMouseButtonPressed(FlMouseButton::Left),
        "quick mouse tap keeps pressed transition");
    Check(FlInput::Instance().IsMouseButtonReleased(FlMouseButton::Left),
        "quick mouse tap keeps released transition");
    FlInput::Instance().EndFrame();

    SendMessageW(window, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), MAKELPARAM(14, 27));
    BeginFrame("wheel frame");
    Check(FlInput::Instance().GetMouseWheelDelta() == FlMouseWheelDetent,
        "wheel exposes one raw detent");
    FlInput::Instance().EndFrame();
    BeginFrame("wheel clear frame");
    Check(FlInput::Instance().GetMouseWheelDelta() == 0, "wheel delta clears next frame");
    Check(FlInput::Instance().GetMouseDelta().x == 0 && FlInput::Instance().GetMouseDelta().y == 0,
        "mouse delta clears next frame");
    FlInput::Instance().EndFrame();

    const auto& runtime = FlInput::RuntimeAPI();
    Check(runtime.IsCompatible(), "runtime API size/version is compatible");
    Check(runtime.GetMouseWheelDelta() == 0, "runtime API reaches host state");
    Check(!runtime.IsKeyDown(static_cast<FlKey>(0xFFFFFFFFu)), "invalid runtime key is safe");
    Check(!runtime.IsMouseButtonDown(static_cast<FlMouseButton>(99)), "invalid runtime mouse button is safe");

    FlInput::Instance().Shutdown();
    Check(!FlInput::Instance().IsInitialized(), "shutdown clears initialized state");
    DestroyWindow(window);
    UnregisterClassW(WindowClassName, instance);

    if (g_failures == 0)
        std::puts("FlInput smoke: PASS");
    return g_failures;
}
