#include "FlTerminalEditor.h"

void FlTerminalEditor::RenderTerminal(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        if (ImGui::BeginChild("Output", ImVec2(Def::FloatZero, -ImGui::GetFrameHeightWithSpacing()), true))
        {
            std::lock_guard<std::mutex> lock(m_logMutex);
            for (const auto& line : m_log)
                ImGui::TextUnformatted(line.c_str());

            if (m_scrollToBottom)
            {
                ImGui::SetScrollHereY(Def::FloatOne);
                m_scrollToBottom = false;
            }
        }
        ImGui::EndChild();

        ImGui::PushItemWidth(-Def::FloatOne);
        if (ImGui::InputText("##Input", &m_inputBuf, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            ExecuteCommand(m_inputBuf.c_str());
            m_inputBuf.clear();
            ImGui::SetKeyboardFocusHere(-Def::IntOne);
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

void FlTerminalEditor::ExecuteCommand(const char* cmd)
{
    if (!cmd || cmd[Def::UIntZero] == '\0') return;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_commandQueue.push(cmd);
    }
    m_cv.notify_one();
}

void FlTerminalEditor::WorkerThread()
{
    while (m_running)
    {
        std::string command;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_cv.wait(lock, [&] { return !m_running || !m_commandQueue.empty(); });
            if (!m_running) break;
            if (!m_commandQueue.empty()) {
                command = m_commandQueue.front();
                m_commandQueue.pop();
            }
        }

        if (command.empty()) continue;
        m_isRunningCommand = true;

        // --- 内部コマンド処理 ---
        if (command.rfind("cd ", 0) == 0) 
        {
            std::string newDir = command.substr(3);
            auto newPath = m_currentDir / newDir;
            try {
                newPath = std::filesystem::canonical(newPath);
                if (std::filesystem::is_directory(newPath)) 
                {
                    m_currentDir = newPath;
                    AddLog("> Changed Current Directory: " + m_currentDir.string());
                }
                else 
                    AddLog("> Directory does not exist: " + newPath.string());
            }
            catch (const std::exception& e) {
                AddLog("> Directory change failed: " + std::string(e.what()));
            }
            continue;
        }
        if (command == "cls")
        {
            {
                std::lock_guard<std::mutex> lock(m_logMutex);
                m_log.clear();
            }
            AddLog("> Terminal Current Directory: " + m_currentDir.string());
            continue;
        }

        // --- 外部コマンド実行 ---
        SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        HANDLE hRead = nullptr, hWrite = nullptr;
        CreatePipe(&hRead, &hWrite, &sa, 0);
        SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFO si{ sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi{};
        si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.wShowWindow = SW_HIDE;

        auto fullCmd = "chcp 65001 > nul && cd /d \"" + m_currentDir.string() + "\" && " + command;
        auto wcmd = L"cmd.exe /c " + ansi_to_wide(fullCmd);

        if (!CreateProcess(nullptr, wcmd.data(), nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            AddLog("> Failed to execute command: " + command);
            continue;
        }

        CloseHandle(hWrite);

        AddLog("> " + m_currentDir.string() + " " + command);

        auto userCmd{ std::string_view{command} };
        auto standardCmd{ std::string_view{"cd "} };

        if (userCmd.substr(0, 3) == standardCmd.substr(0, 3))
        {
            auto newDir{ std::string{ command.c_str() + 3}};
            auto newPath{ m_currentDir / newDir };

            try {
                newPath = std::filesystem::canonical(newPath);
                if (std::filesystem::is_directory(newPath))
                {
                    m_currentDir = newPath;
                    AddLog("> Changed Current Directory: " + m_currentDir.string());
                }
                else AddLog("> Directory Does Not Exist: " + newPath.string());
            }
            catch (const std::exception& e) {
                AddLog("> Directory Move Failed: " + std::string(e.what()));
            }

            m_scrollToBottom = true;
            return;
        }

        // --- リアルタイム読み取り ---
        char buffer[4096];
        DWORD bytesRead = 0;

        while (true) {
            BOOL success = ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
            if (!success || bytesRead == 0) {
                // 出力が終わったら break
                if (WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0)
                    break;
                continue;
            }
            buffer[bytesRead] = '\0';
            AddLog(std::string(buffer, bytesRead));
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode{ Def::ULongZero };
        if (GetExitCodeProcess(pi.hProcess, &exitCode))
        {
            if (exitCode == Def::ULongZero)
            {
                m_isRunningCommand = false;
                AddLog("> Process Completed Successfully. (ExitCode=0)");
            }
            else
            {
                m_isRunningCommand = false;
                AddLog("> Process Completed with Errors. ExitCode=" + std::to_string(exitCode));
            }
        }
        else
        {
            m_isRunningCommand = false;
            AddLog("> Failed to retrieve process exit code.");
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hRead);

        m_scrollToBottom = true;
    }
}

void FlTerminalEditor::AddLog(const std::string& log)
{
    auto processedLog{ log };
    auto pos{ Def::ULongLongZero };
    while ((pos = processedLog.find("\r\n", pos)) != std::string::npos) {
        processedLog.replace(pos, 2, "\n");
        ++pos;
    }

    std::stringstream ss(processedLog);
    std::string line;

    auto deb{ std::make_unique<DebugLogger>("Assets/Data/Log/Command.log") };

    std::lock_guard<std::mutex> lock(m_logMutex);
    while (std::getline(ss, line, '\n')) {
        if (Str::IsLikelyUtf8(line))
            m_log.push_back(line);
        else
            m_log.push_back(sjis_to_utf8(line));

        DEBUG_LOG(deb, line);
    }
    m_scrollToBottom = true;
}
