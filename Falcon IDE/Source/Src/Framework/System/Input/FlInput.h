#pragma once

#include "../../Module/RuntimeModule/Input.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <Keyboard.h>
#include <Mouse.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

// Host-side input service. ProcessMessage and BeginFrame must run on the window thread.
class FlInput final
{
public:
    static FlInput& Instance() noexcept;

    [[nodiscard]] bool Initialize(void* windowHandle) noexcept;
    void Shutdown() noexcept;
    void Reset() noexcept;

    void ProcessMessage(std::uint32_t message, std::uintptr_t wParam, std::intptr_t lParam) noexcept;
    [[nodiscard]] bool BeginFrame() noexcept;
    void EndFrame() noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept { return m_isInitialized; }

    [[nodiscard]] bool IsKeyDown(FlKey key) const noexcept;
    [[nodiscard]] bool IsKeyPressed(FlKey key) const noexcept;
    [[nodiscard]] bool IsKeyReleased(FlKey key) const noexcept;

    [[nodiscard]] bool IsMouseButtonDown(FlMouseButton button) const noexcept;
    [[nodiscard]] bool IsMouseButtonPressed(FlMouseButton button) const noexcept;
    [[nodiscard]] bool IsMouseButtonReleased(FlMouseButton button) const noexcept;

    [[nodiscard]] FlInputVector2 GetMousePosition() const noexcept;
    [[nodiscard]] FlInputVector2 GetMouseDelta() const noexcept;
    [[nodiscard]] std::int32_t GetMouseWheelDelta() const noexcept;

    [[nodiscard]] static const FlInputAPI& RuntimeAPI() noexcept;

private:
    static constexpr std::size_t KeyCount = 256;
    static constexpr std::size_t MouseButtonCount = 5;

    using KeyFlags = std::array<std::uint8_t, KeyCount>;
    using MouseButtonFlags = std::array<std::uint8_t, MouseButtonCount>;

    FlInput() = default;
    ~FlInput() = default;
    FlInput(const FlInput&) = delete;
    FlInput& operator=(const FlInput&) = delete;

    void CaptureKeyboardTransitions();
    void CaptureMouseTransitions();

    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;

    DirectX::Keyboard::State m_keyboardState{};
    DirectX::Keyboard::State m_messageKeyboardState{};
    DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker{};

    DirectX::Mouse::State m_mouseState{};
    DirectX::Mouse::State m_messageMouseState{};
    DirectX::Mouse::ButtonStateTracker m_mouseTracker{};

    KeyFlags m_pendingKeyPressed{};
    KeyFlags m_pendingKeyReleased{};
    KeyFlags m_keyPressed{};
    KeyFlags m_keyReleased{};

    MouseButtonFlags m_pendingMousePressed{};
    MouseButtonFlags m_pendingMouseReleased{};
    MouseButtonFlags m_mousePressed{};
    MouseButtonFlags m_mouseReleased{};

    FlInputVector2 m_mousePosition{};
    FlInputVector2 m_mouseDelta{};
    std::int32_t m_mouseWheelDelta{};

    bool m_hasMessageKeyboardState{};
    bool m_hasMouseState{};
    bool m_hasMessageMouseState{};
    bool m_waitingForAbsolutePosition{};
    bool m_isInitialized{};
};
