//#pragma once
//
//class FlInputManager
//{
//public:
//    enum class DeviceType { Keyboard, Mouse, Gamepad };
//
//    struct ActionKey
//    {
//        DeviceType device;
//        int code;   // キーコード or ボタンコード
//        int index;  // ゲームパッド番号（Keyboard, Mouseなら0固定）
//    };
//
//    static FlInputManager& Instance()
//    {
//        static FlInputManager instance;
//        return instance;
//    }
//
//    void Update() noexcept;
//
//    // ------------------------
//    // アクション登録
//    // ------------------------
//    void BindAction(const std::string& action, DeviceType device, int code, int index = 0);
//
//    // アクション判定
//    bool IsActionDown(const std::string& action) const noexcept;
//
//    bool IsActionPressed(const std::string& action) const noexcept;
//
//    bool IsActionReleased(const std::string& action) const noexcept;
//
//    // ------------------------
//    // デバイス状態の直接取得（従来機能）
//    // ------------------------
//    bool IsKeyDown(DirectX::Keyboard::Keys key) const noexcept
//    {
//        return m_keyboardState.IsKeyDown(key);
//    }
//
//    bool IsKeyPressed(DirectX::Keyboard::Keys key) const noexcept
//    {
//        return m_keyboardTracker.IsKeyPressed(key);
//    }
//
//    bool IsKeyReleased(DirectX::Keyboard::Keys key) const noexcept
//    {
//        return m_keyboardTracker.IsKeyReleased(key);
//    }
//
//    bool IsLeftButtonDown()  const noexcept { return m_mouseState.leftButton; }
//    bool IsRightButtonDown() const noexcept { return m_mouseState.rightButton; }
//    int GetMouseX() const noexcept { return m_mouseState.x; }
//    int GetMouseY() const noexcept { return m_mouseState.y; }
//
//private:
//    FlInputManager()
//        : m_keyboard{ std::make_unique<DirectX::Keyboard>() }
//        , m_mouse{ std::make_unique<DirectX::Mouse>() }
//    {
//        m_mouse->SetWindow(nullptr);
//    }
//
//    bool CheckDown(const ActionKey& key) const noexcept
//    {
//        switch (key.device)
//        {
//        case DeviceType::Keyboard: return m_keyboardState.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(key.code));
//        case DeviceType::Mouse:    return (key.code == 0) ? m_mouseState.leftButton : false; 
//        //case DeviceType::Gamepad:  return m_gamepadConnected[key.index] && (m_gamepadStates[key.index].wButtons & key.code);
//        }
//        return false;
//    }
//
//    bool CheckPressed(const ActionKey& key) const noexcept
//    {
//        switch (key.device)
//        {
//        case DeviceType::Keyboard: return m_keyboardTracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(key.code));
//        case DeviceType::Mouse:    return (key.code == 0) ? (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::PRESSED) : false;
//        //case DeviceType::Gamepad:  return m_gamepadConnected[key.index] && (m_gamepadStates[key.index].wButtons & key.code); 
//        }
//        return false;
//    }
//
//    bool CheckReleased(const ActionKey& key) const noexcept
//    {
//        switch (key.device)
//        {
//        case DeviceType::Keyboard: return m_keyboardTracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(key.code));
//        case DeviceType::Mouse:    return (key.code == 0) ? (m_mouseTracker.leftButton == DirectX::Mouse::ButtonStateTracker::RELEASED) : false;
//        case DeviceType::Gamepad:  return false;
//        }
//        return false;
//    }
//
//private:
//    // キーボード
//    std::unique_ptr<DirectX::Keyboard> m_keyboard;
//    DirectX::Keyboard::State m_keyboardState{};
//    DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker{};
//
//    // マウス
//    std::unique_ptr<DirectX::Mouse> m_mouse;
//    DirectX::Mouse::State m_mouseState{};
//    DirectX::Mouse::ButtonStateTracker m_mouseTracker{};
//
//    // ゲームパッド
//    //std::array<XINPUT_GAMEPAD, 4> m_gamepadStates{};
//    std::array<bool, 4> m_gamepadConnected{};
//
//    // アクションマッピング
//    std::unordered_map<std::string, std::vector<ActionKey>> m_actionMap;
//};
