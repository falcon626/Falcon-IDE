#include "FlInput.h"

namespace
{
    constexpr std::size_t MouseButtonCount = 5;

    [[nodiscard]] bool IsKeyboardTransitionMessage(std::uint32_t message) noexcept
    {
        switch (message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsMouseTransitionMessage(std::uint32_t message) noexcept
    {
        switch (message)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsFocusTransitionMessage(std::uint32_t message) noexcept
    {
        return message == WM_ACTIVATE || message == WM_ACTIVATEAPP;
    }

    [[nodiscard]] bool IsAbsolutePositionMessage(std::uint32_t message) noexcept
    {
        switch (message)
        {
        case WM_MOUSEMOVE:
        case WM_MOUSEHOVER:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsFocusLossMessage(std::uint32_t message, std::uintptr_t wParam) noexcept
    {
        if (message == WM_ACTIVATEAPP)
            return wParam == FALSE;
        return message == WM_ACTIVATE && LOWORD(static_cast<WPARAM>(wParam)) == WA_INACTIVE;
    }

    [[nodiscard]] bool MouseButtonDown(const DirectX::Mouse::State& state, std::size_t button) noexcept
    {
        switch (button)
        {
        case 0: return state.leftButton;
        case 1: return state.middleButton;
        case 2: return state.rightButton;
        case 3: return state.xButton1;
        case 4: return state.xButton2;
        default: return false;
        }
    }

    [[nodiscard]] DirectX::Mouse::ButtonStateTracker::ButtonState MouseButtonState(
        const DirectX::Mouse::ButtonStateTracker& tracker, std::size_t button) noexcept
    {
        switch (button)
        {
        case 0: return tracker.leftButton;
        case 1: return tracker.middleButton;
        case 2: return tracker.rightButton;
        case 3: return tracker.xButton1;
        case 4: return tracker.xButton2;
        default: return DirectX::Mouse::ButtonStateTracker::UP;
        }
    }

    [[nodiscard]] std::uint8_t __cdecl RuntimeKeyDown(std::uint32_t key) noexcept
    {
        return FlInput::Instance().IsKeyDown(static_cast<FlKey>(key)) ? 1u : 0u;
    }

    [[nodiscard]] std::uint8_t __cdecl RuntimeKeyPressed(std::uint32_t key) noexcept
    {
        return FlInput::Instance().IsKeyPressed(static_cast<FlKey>(key)) ? 1u : 0u;
    }

    [[nodiscard]] std::uint8_t __cdecl RuntimeKeyReleased(std::uint32_t key) noexcept
    {
        return FlInput::Instance().IsKeyReleased(static_cast<FlKey>(key)) ? 1u : 0u;
    }

    [[nodiscard]] std::uint8_t __cdecl RuntimeMouseButtonDown(std::uint32_t button) noexcept
    {
        return FlInput::Instance().IsMouseButtonDown(static_cast<FlMouseButton>(button)) ? 1u : 0u;
    }

    [[nodiscard]] std::uint8_t __cdecl RuntimeMouseButtonPressed(std::uint32_t button) noexcept
    {
        return FlInput::Instance().IsMouseButtonPressed(static_cast<FlMouseButton>(button)) ? 1u : 0u;
    }

    [[nodiscard]] std::uint8_t __cdecl RuntimeMouseButtonReleased(std::uint32_t button) noexcept
    {
        return FlInput::Instance().IsMouseButtonReleased(static_cast<FlMouseButton>(button)) ? 1u : 0u;
    }

    [[nodiscard]] std::int32_t __cdecl RuntimeMouseX() noexcept
    {
        return FlInput::Instance().GetMousePosition().x;
    }

    [[nodiscard]] std::int32_t __cdecl RuntimeMouseY() noexcept
    {
        return FlInput::Instance().GetMousePosition().y;
    }

    [[nodiscard]] std::int32_t __cdecl RuntimeMouseDeltaX() noexcept
    {
        return FlInput::Instance().GetMouseDelta().x;
    }

    [[nodiscard]] std::int32_t __cdecl RuntimeMouseDeltaY() noexcept
    {
        return FlInput::Instance().GetMouseDelta().y;
    }

    [[nodiscard]] std::int32_t __cdecl RuntimeMouseWheelDelta() noexcept
    {
        return FlInput::Instance().GetMouseWheelDelta();
    }
}

FlInput& FlInput::Instance() noexcept
{
    static FlInput instance;
    return instance;
}

bool FlInput::Initialize(void* windowHandle) noexcept
{
    if (!windowHandle)
        return false;

    Shutdown();

    try
    {
        auto keyboard = std::make_unique<DirectX::Keyboard>();
        auto mouse = std::make_unique<DirectX::Mouse>();
        mouse->SetWindow(static_cast<HWND>(windowHandle));

        m_keyboard = std::move(keyboard);
        m_mouse = std::move(mouse);
        m_isInitialized = true;
        Reset();
        return true;
    }
    catch (...)
    {
        Shutdown();
        return false;
    }
}

void FlInput::Shutdown() noexcept
{
    if (m_isInitialized)
        Reset();

    m_mouse.reset();
    m_keyboard.reset();
    m_isInitialized = false;
}

void FlInput::Reset() noexcept
{
    if (m_keyboard)
        m_keyboard->Reset();

    if (m_mouse)
    {
        try
        {
            m_mouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
            m_mouse->ResetScrollWheelValue();
        }
        catch (...)
        {
            // The public state is still reset below; BeginFrame reports device failures.
        }
    }

    m_keyboardState = {};
    m_messageKeyboardState = {};
    m_keyboardTracker.Reset();
    m_mouseState = {};
    m_messageMouseState = {};
    m_mouseTracker.Reset();

    m_pendingKeyPressed.fill(0);
    m_pendingKeyReleased.fill(0);
    m_keyPressed.fill(0);
    m_keyReleased.fill(0);
    m_pendingMousePressed.fill(0);
    m_pendingMouseReleased.fill(0);
    m_mousePressed.fill(0);
    m_mouseReleased.fill(0);

    m_mousePosition = {};
    m_mouseDelta = {};
    m_mouseWheelDelta = 0;
    m_hasMessageKeyboardState = false;
    m_hasMouseState = false;
    m_hasMessageMouseState = false;
    m_waitingForAbsolutePosition = true;
}

void FlInput::ProcessMessage(std::uint32_t message, std::uintptr_t wParam, std::intptr_t lParam) noexcept
{
    if (!m_isInitialized)
        return;

    try
    {
        DirectX::Keyboard::ProcessMessage(
            static_cast<UINT>(message), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
        DirectX::Mouse::ProcessMessage(
            static_cast<UINT>(message), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));

        // DirectXTK resets on app deactivation. Mirror that behavior when this
        // top-level window alone is deactivated (for example by a modal dialog).
        if (message == WM_ACTIVATE && IsFocusLossMessage(message, wParam))
        {
            DirectX::Keyboard::ProcessMessage(WM_ACTIVATEAPP, FALSE, 0);
            DirectX::Mouse::ProcessMessage(WM_ACTIVATEAPP, FALSE, 0);
        }

        if (IsKeyboardTransitionMessage(message))
            CaptureKeyboardTransitions();
        if (IsMouseTransitionMessage(message))
            CaptureMouseTransitions();
        if (IsFocusTransitionMessage(message))
        {
            // DirectXTK can clear absolute coordinates while inactive. The first
            // coordinate after focus returns becomes a baseline, never a fake jump.
            m_hasMouseState = false;
            m_waitingForAbsolutePosition = true;
            m_mouseState.scrollWheelValue = m_messageMouseState.scrollWheelValue;
        }
        else if (IsAbsolutePositionMessage(message) && m_waitingForAbsolutePosition)
        {
            m_hasMouseState = false;
            m_waitingForAbsolutePosition = false;
        }
    }
    catch (...)
    {
        Reset();
    }
}

bool FlInput::BeginFrame() noexcept
{
    if (!m_isInitialized || !m_keyboard || !m_mouse)
        return false;

    try
    {
        const auto keyboardState = m_keyboard->GetState();
        const auto mouseState = m_mouse->GetState();

        m_keyboardTracker.Update(keyboardState);
        for (std::size_t key = 0; key < KeyCount; ++key)
        {
            const auto directXKey = static_cast<DirectX::Keyboard::Keys>(key);
            m_keyPressed[key] = static_cast<std::uint8_t>(
                m_pendingKeyPressed[key] != 0 || m_keyboardTracker.IsKeyPressed(directXKey));
            m_keyReleased[key] = static_cast<std::uint8_t>(
                m_pendingKeyReleased[key] != 0 || m_keyboardTracker.IsKeyReleased(directXKey));
        }

        m_mouseTracker.Update(mouseState);
        for (std::size_t button = 0; button < MouseButtonCount; ++button)
        {
            const auto state = MouseButtonState(m_mouseTracker, button);
            m_mousePressed[button] = static_cast<std::uint8_t>(
                m_pendingMousePressed[button] != 0 || state == DirectX::Mouse::ButtonStateTracker::PRESSED);
            m_mouseReleased[button] = static_cast<std::uint8_t>(
                m_pendingMouseReleased[button] != 0 || state == DirectX::Mouse::ButtonStateTracker::RELEASED);
        }

        m_mouseWheelDelta = mouseState.scrollWheelValue - m_mouseState.scrollWheelValue;
        if (m_hasMouseState && !m_waitingForAbsolutePosition)
        {
            m_mouseDelta = mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE
                ? FlInputVector2{ mouseState.x, mouseState.y }
                : FlInputVector2{ mouseState.x - m_mouseState.x, mouseState.y - m_mouseState.y };
        }
        else
        {
            m_mouseDelta = {};
        }

        m_keyboardState = keyboardState;
        m_messageKeyboardState = keyboardState;
        if (!m_waitingForAbsolutePosition)
            m_mousePosition = { mouseState.x, mouseState.y };
        m_mouseState = mouseState;
        m_messageMouseState = mouseState;

        m_pendingKeyPressed.fill(0);
        m_pendingKeyReleased.fill(0);
        m_pendingMousePressed.fill(0);
        m_pendingMouseReleased.fill(0);
        m_hasMessageKeyboardState = true;
        m_hasMouseState = !m_waitingForAbsolutePosition;
        m_hasMessageMouseState = true;
        return true;
    }
    catch (...)
    {
        Reset();
        return false;
    }
}

void FlInput::EndFrame() noexcept
{
    if (m_mouse)
        m_mouse->EndOfInputFrame();
}

bool FlInput::IsKeyDown(FlKey key) const noexcept
{
    const auto value = static_cast<std::uint32_t>(key);
    return m_isInitialized && value < KeyCount
        && m_keyboardState.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(value));
}

bool FlInput::IsKeyPressed(FlKey key) const noexcept
{
    const auto value = static_cast<std::uint32_t>(key);
    return m_isInitialized && value < KeyCount && m_keyPressed[value] != 0;
}

bool FlInput::IsKeyReleased(FlKey key) const noexcept
{
    const auto value = static_cast<std::uint32_t>(key);
    return m_isInitialized && value < KeyCount && m_keyReleased[value] != 0;
}

bool FlInput::IsMouseButtonDown(FlMouseButton button) const noexcept
{
    const auto value = static_cast<std::uint32_t>(button);
    return m_isInitialized && value < MouseButtonCount && MouseButtonDown(m_mouseState, value);
}

bool FlInput::IsMouseButtonPressed(FlMouseButton button) const noexcept
{
    const auto value = static_cast<std::uint32_t>(button);
    return m_isInitialized && value < MouseButtonCount && m_mousePressed[value] != 0;
}

bool FlInput::IsMouseButtonReleased(FlMouseButton button) const noexcept
{
    const auto value = static_cast<std::uint32_t>(button);
    return m_isInitialized && value < MouseButtonCount && m_mouseReleased[value] != 0;
}

FlInputVector2 FlInput::GetMousePosition() const noexcept
{
    return m_mousePosition;
}

FlInputVector2 FlInput::GetMouseDelta() const noexcept
{
    return m_mouseDelta;
}

std::int32_t FlInput::GetMouseWheelDelta() const noexcept
{
    return m_mouseWheelDelta;
}

const FlInputAPI& FlInput::RuntimeAPI() noexcept
{
    static const FlInputAPI api{
        static_cast<std::uint32_t>(sizeof(FlInputAPI)),
        FlInputAPIVersion,
        &RuntimeKeyDown,
        &RuntimeKeyPressed,
        &RuntimeKeyReleased,
        &RuntimeMouseButtonDown,
        &RuntimeMouseButtonPressed,
        &RuntimeMouseButtonReleased,
        &RuntimeMouseX,
        &RuntimeMouseY,
        &RuntimeMouseDeltaX,
        &RuntimeMouseDeltaY,
        &RuntimeMouseWheelDelta,
    };
    return api;
}

void FlInput::CaptureKeyboardTransitions()
{
    if (!m_keyboard)
        return;

    const auto current = m_keyboard->GetState();
    for (std::size_t key = 0; key < KeyCount; ++key)
    {
        const auto directXKey = static_cast<DirectX::Keyboard::Keys>(key);
        const bool wasDown = m_hasMessageKeyboardState && m_messageKeyboardState.IsKeyDown(directXKey);
        const bool isDown = current.IsKeyDown(directXKey);
        if (!wasDown && isDown)
            m_pendingKeyPressed[key] = 1;
        else if (wasDown && !isDown)
            m_pendingKeyReleased[key] = 1;
    }

    m_messageKeyboardState = current;
    m_hasMessageKeyboardState = true;
}

void FlInput::CaptureMouseTransitions()
{
    if (!m_mouse)
        return;

    const auto current = m_mouse->GetState();
    for (std::size_t button = 0; button < MouseButtonCount; ++button)
    {
        const bool wasDown = m_hasMessageMouseState && MouseButtonDown(m_messageMouseState, button);
        const bool isDown = MouseButtonDown(current, button);
        if (!wasDown && isDown)
            m_pendingMousePressed[button] = 1;
        else if (wasDown && !isDown)
            m_pendingMouseReleased[button] = 1;
    }

    m_messageMouseState = current;
    m_hasMessageMouseState = true;
}
