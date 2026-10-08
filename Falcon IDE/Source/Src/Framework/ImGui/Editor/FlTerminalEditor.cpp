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

std::future<bool> FlTerminalEditor::ExecuteCommand(const char* cmd)
{
    CommandJob job{ cmd ? cmd : "", {} };
    auto completion = job.completion.get_future();
    if (job.command.empty())
    {
        job.completion.set_value(false);
        return completion;
    }
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_running)
        {
            job.completion.set_value(false);
            return completion;
        }
        m_commandQueue.push(std::move(job));
    }
    m_cv.notify_one();
    return completion;
}

void FlTerminalEditor::WorkerThread()
{
    while (m_running)
    {
        CommandJob job;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_cv.wait(lock, [&] { return !m_running || !m_commandQueue.empty(); });
            if (!m_running) break;
            job = std::move(m_commandQueue.front());
            m_commandQueue.pop();
        }
        const auto& command = job.command;
        m_isRunningCommand = true;
        const auto finish = [&](bool success) {
            m_isRunningCommand = false;
            job.completion.set_value(success);
        };

        if (command.rfind("cd ", 0) == 0)
        {
            try {
                auto newPath = std::filesystem::canonical(m_currentDir / command.substr(3));
                if (!std::filesystem::is_directory(newPath)) throw std::runtime_error("Not a directory");
                m_currentDir = newPath;
                AddLog("> Changed Current Directory: " + m_currentDir.string());
                finish(true);
            }
            catch (const std::exception& e) {
                AddLog("> Directory change failed: " + std::string(e.what()));
                finish(false);
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
            finish(true);
            continue;
        }

        SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        HANDLE hRead = nullptr, hWrite = nullptr;
        if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        {
            AddLog("> Failed to create command output pipe.");
            finish(false);
            continue;
        }
        if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
        {
            CloseHandle(hRead);
            CloseHandle(hWrite);
            AddLog("> Failed to configure command output pipe.");
            finish(false);
            continue;
        }

        STARTUPINFOW si{ sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION pi{};
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.wShowWindow = SW_HIDE;
        auto fullCmd = "chcp 65001 > nul && cd /d \"" + m_currentDir.string() + "\" && " + command;
        auto wcmd = L"cmd.exe /c " + ansi_to_wide(fullCmd);
        bool launched = false;
        {
            std::lock_guard<std::mutex> lock(m_processMutex);
            if (m_running && !m_processJob)
            {
                m_processJob = CreateJobObjectW(nullptr, nullptr);
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
                limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                if (m_processJob && !SetInformationJobObject(m_processJob,
                    JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
                {
                    CloseHandle(m_processJob);
                    m_processJob = nullptr;
                }
            }
            if (m_running && m_processJob)
            {
                launched = CreateProcessW(nullptr, wcmd.data(), nullptr, nullptr, TRUE,
                    CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi) != FALSE;
                if (launched && (!AssignProcessToJobObject(m_processJob, pi.hProcess) ||
                    ResumeThread(pi.hThread) == static_cast<DWORD>(-1)))
                {
                    TerminateProcess(pi.hProcess, 1);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    launched = false;
                }
            }
        }
        CloseHandle(hWrite);
        if (!launched)
        {
            CloseHandle(hRead);
            AddLog("> Failed to execute command: " + command);
            finish(false);
            continue;
        }

        AddLog("> " + m_currentDir.string() + " " + command);
        char buffer[4096];
        DWORD bytesRead = 0;
        while (m_running && ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
            AddLog(std::string(buffer, bytesRead));

        WaitForSingleObject(pi.hProcess, m_running ? INFINITE : 5000);
        DWORD exitCode{};
        const auto gotExitCode = GetExitCodeProcess(pi.hProcess, &exitCode);
        if (!gotExitCode)
            AddLog("> Failed to retrieve process exit code.");
        else if (exitCode == 0)
            AddLog("> Process Completed Successfully. (ExitCode=0)");
        else
            AddLog("> Process Completed with Errors. ExitCode=" + std::to_string(exitCode));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hRead);
        finish(m_running && gotExitCode && exitCode == 0);
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
