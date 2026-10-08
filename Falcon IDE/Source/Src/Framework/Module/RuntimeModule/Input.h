#pragma once

#include <cstdint>

// Stable input codes shared by the executable and ScriptModule DLLs.
// Keyboard values intentionally match the Win32 virtual-key values used by DirectXTK.
enum class FlKey : std::uint32_t
{
    None = 0x00,
    Backspace = 0x08,
    Back = Backspace,
    Tab = 0x09,
    Enter = 0x0D,
    Return = Enter,
    Pause = 0x13,
    CapsLock = 0x14,
    Kana = 0x15,
    ImeOn = 0x16,
    Kanji = 0x19,
    ImeOff = 0x1A,
    Escape = 0x1B,
    ImeConvert = 0x1C,
    ImeNoConvert = 0x1D,
    Space = 0x20,
    PageUp = 0x21,
    PageDown = 0x22,
    End = 0x23,
    Home = 0x24,
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,
    Select = 0x29,
    Print = 0x2A,
    Execute = 0x2B,
    PrintScreen = 0x2C,
    Insert = 0x2D,
    Delete = 0x2E,
    Help = 0x2F,
    D0 = 0x30,
    D1 = 0x31,
    D2 = 0x32,
    D3 = 0x33,
    D4 = 0x34,
    D5 = 0x35,
    D6 = 0x36,
    D7 = 0x37,
    D8 = 0x38,
    D9 = 0x39,
    A = 0x41,
    B = 0x42,
    C = 0x43,
    D = 0x44,
    E = 0x45,
    F = 0x46,
    G = 0x47,
    H = 0x48,
    I = 0x49,
    J = 0x4A,
    K = 0x4B,
    L = 0x4C,
    M = 0x4D,
    N = 0x4E,
    O = 0x4F,
    P = 0x50,
    Q = 0x51,
    R = 0x52,
    S = 0x53,
    T = 0x54,
    U = 0x55,
    V = 0x56,
    W = 0x57,
    X = 0x58,
    Y = 0x59,
    Z = 0x5A,
    LeftWindows = 0x5B,
    RightWindows = 0x5C,
    Apps = 0x5D,
    Sleep = 0x5F,
    NumPad0 = 0x60,
    NumPad1 = 0x61,
    NumPad2 = 0x62,
    NumPad3 = 0x63,
    NumPad4 = 0x64,
    NumPad5 = 0x65,
    NumPad6 = 0x66,
    NumPad7 = 0x67,
    NumPad8 = 0x68,
    NumPad9 = 0x69,
    Multiply = 0x6A,
    Add = 0x6B,
    Separator = 0x6C,
    Subtract = 0x6D,
    Decimal = 0x6E,
    Divide = 0x6F,
    F1 = 0x70,
    F2 = 0x71,
    F3 = 0x72,
    F4 = 0x73,
    F5 = 0x74,
    F6 = 0x75,
    F7 = 0x76,
    F8 = 0x77,
    F9 = 0x78,
    F10 = 0x79,
    F11 = 0x7A,
    F12 = 0x7B,
    F13 = 0x7C,
    F14 = 0x7D,
    F15 = 0x7E,
    F16 = 0x7F,
    F17 = 0x80,
    F18 = 0x81,
    F19 = 0x82,
    F20 = 0x83,
    F21 = 0x84,
    F22 = 0x85,
    F23 = 0x86,
    F24 = 0x87,
    NumLock = 0x90,
    Scroll = 0x91,
    LeftShift = 0xA0,
    RightShift = 0xA1,
    LeftControl = 0xA2,
    RightControl = 0xA3,
    LeftAlt = 0xA4,
    RightAlt = 0xA5,
    BrowserBack = 0xA6,
    BrowserForward = 0xA7,
    BrowserRefresh = 0xA8,
    BrowserStop = 0xA9,
    BrowserSearch = 0xAA,
    BrowserFavorites = 0xAB,
    BrowserHome = 0xAC,
    VolumeMute = 0xAD,
    VolumeDown = 0xAE,
    VolumeUp = 0xAF,
    MediaNextTrack = 0xB0,
    MediaPreviousTrack = 0xB1,
    MediaStop = 0xB2,
    MediaPlayPause = 0xB3,
    LaunchMail = 0xB4,
    SelectMedia = 0xB5,
    LaunchApplication1 = 0xB6,
    LaunchApplication2 = 0xB7,
    OemSemicolon = 0xBA,
    OemPlus = 0xBB,
    OemComma = 0xBC,
    OemMinus = 0xBD,
    OemPeriod = 0xBE,
    OemQuestion = 0xBF,
    OemTilde = 0xC0,
    OemOpenBrackets = 0xDB,
    OemPipe = 0xDC,
    OemCloseBrackets = 0xDD,
    OemQuotes = 0xDE,
    Oem8 = 0xDF,
    OemBackslash = 0xE2,
    ProcessKey = 0xE5,
    OemCopy = 0xF2,
    OemAuto = 0xF3,
    OemEnlW = 0xF4,
    Attn = 0xF6,
    Crsel = 0xF7,
    Exsel = 0xF8,
    EraseEof = 0xF9,
    Play = 0xFA,
    Zoom = 0xFB,
    Pa1 = 0xFD,
    OemClear = 0xFE,
};

enum class FlMouseButton : std::uint32_t
{
    Left = 0,
    Middle,
    Right,
    X1,
    X2,
};

struct FlInputVector2
{
    std::int32_t x{};
    std::int32_t y{};
};

inline constexpr std::int32_t FlMouseWheelDetent = 120;
// Versions are additive: consumers accept this version or any newer table that
// retains the v1 prefix and reports at least sizeof(FlInputAPI).
inline constexpr std::uint32_t FlInputAPIVersion = 1;

#if defined(_MSC_VER)
#define FL_INPUT_CALL __cdecl
#else
#define FL_INPUT_CALL
#endif

// Host-owned function table. ScriptModule DLLs may call it, but must not retain
// host frame data or mutate/free the table.
struct FlInputAPI
{
    std::uint32_t size;
    std::uint32_t version;

    std::uint8_t(FL_INPUT_CALL* KeyDown)(std::uint32_t key);
    std::uint8_t(FL_INPUT_CALL* KeyPressed)(std::uint32_t key);
    std::uint8_t(FL_INPUT_CALL* KeyReleased)(std::uint32_t key);

    std::uint8_t(FL_INPUT_CALL* MouseButtonDown)(std::uint32_t button);
    std::uint8_t(FL_INPUT_CALL* MouseButtonPressed)(std::uint32_t button);
    std::uint8_t(FL_INPUT_CALL* MouseButtonReleased)(std::uint32_t button);

    std::int32_t(FL_INPUT_CALL* MouseX)();
    std::int32_t(FL_INPUT_CALL* MouseY)();
    std::int32_t(FL_INPUT_CALL* MouseDeltaX)();
    std::int32_t(FL_INPUT_CALL* MouseDeltaY)();
    std::int32_t(FL_INPUT_CALL* MouseWheelDelta)();

    [[nodiscard]] bool IsCompatible() const noexcept
    {
        return size >= sizeof(FlInputAPI) && version >= FlInputAPIVersion;
    }

    [[nodiscard]] bool IsKeyDown(FlKey key) const noexcept
    {
        return IsCompatible() && KeyDown && KeyDown(static_cast<std::uint32_t>(key)) != 0;
    }

    [[nodiscard]] bool IsKeyPressed(FlKey key) const noexcept
    {
        return IsCompatible() && KeyPressed && KeyPressed(static_cast<std::uint32_t>(key)) != 0;
    }

    [[nodiscard]] bool IsKeyReleased(FlKey key) const noexcept
    {
        return IsCompatible() && KeyReleased && KeyReleased(static_cast<std::uint32_t>(key)) != 0;
    }

    [[nodiscard]] bool IsMouseButtonDown(FlMouseButton button) const noexcept
    {
        return IsCompatible() && MouseButtonDown && MouseButtonDown(static_cast<std::uint32_t>(button)) != 0;
    }

    [[nodiscard]] bool IsMouseButtonPressed(FlMouseButton button) const noexcept
    {
        return IsCompatible() && MouseButtonPressed && MouseButtonPressed(static_cast<std::uint32_t>(button)) != 0;
    }

    [[nodiscard]] bool IsMouseButtonReleased(FlMouseButton button) const noexcept
    {
        return IsCompatible() && MouseButtonReleased && MouseButtonReleased(static_cast<std::uint32_t>(button)) != 0;
    }

    [[nodiscard]] FlInputVector2 GetMousePosition() const noexcept
    {
        return IsCompatible() && MouseX && MouseY
            ? FlInputVector2{ MouseX(), MouseY() }
            : FlInputVector2{};
    }

    [[nodiscard]] FlInputVector2 GetMouseDelta() const noexcept
    {
        return IsCompatible() && MouseDeltaX && MouseDeltaY
            ? FlInputVector2{ MouseDeltaX(), MouseDeltaY() }
            : FlInputVector2{};
    }

    // Raw Win32 wheel units. One standard wheel detent is FlMouseWheelDetent.
    [[nodiscard]] std::int32_t GetMouseWheelDelta() const noexcept
    {
        return IsCompatible() && MouseWheelDelta ? MouseWheelDelta() : 0;
    }
};

#undef FL_INPUT_CALL
