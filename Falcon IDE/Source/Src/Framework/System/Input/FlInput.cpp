//#include "FlInput.h"
//
//void FlInputManager::Update() noexcept
//{
//    // キーボード
//    m_keyboardState = m_keyboard->GetState();
//    m_keyboardTracker.Update(m_keyboardState);
//
//    // マウス
//    m_mouseState = m_mouse->GetState();
//    m_mouseTracker.Update(m_mouseState);
//
//    // ゲームパッド
//    //for (auto i{ Def::ULongLongZero }; i < m_gamepadStates.size(); ++i) {
//    //    XINPUT_STATE state{};
//    //    if (XInputGetState(static_cast<DWORD>(i), &state) == ERROR_SUCCESS)
//    //    {
//    //        m_gamepadConnected[i] = true;
//    //        m_gamepadStates[i] = state.Gamepad;
//    //    }
//    //    else
//    //    {
//    //        m_gamepadConnected[i] = false;
//    //        m_gamepadStates[i] = {};
//    //    }
//    //}
//}
//
//void FlInputManager::BindAction(const std::string& action, DeviceType device, int code, int index)
//{
//    m_actionMap[action].push_back(ActionKey{ device, code, index });
//}
//
//bool FlInputManager::IsActionDown(const std::string& action) const noexcept
//{
//    if (!m_actionMap.contains(action)) return false;
//    for (auto& binding : m_actionMap.at(action)) {
//        if (CheckDown(binding)) return true;
//    }
//    return false;
//}
//
//bool FlInputManager::IsActionPressed(const std::string& action) const noexcept
//{
//    if (!m_actionMap.contains(action)) return false;
//    for (auto& binding : m_actionMap.at(action)) {
//        if (CheckPressed(binding)) return true;
//    }
//    return false;
//}
//
//bool FlInputManager::IsActionReleased(const std::string& action) const noexcept
//{
//    if (!m_actionMap.contains(action)) return false;
//    for (auto& binding : m_actionMap.at(action)) {
//        if (CheckReleased(binding)) return true;
//    }
//    return false;
//}
