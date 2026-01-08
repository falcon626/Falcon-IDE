#pragma once

class FlPythonMacroEditor;
class FlDeveloperCommandPromptEditor;
class FlScriptModuleEditor;

class FlTerminalEditor
{
public:
    FlTerminalEditor() 
    {
        m_log.push_back("> Terminal Current Directory: " + m_currentDir.string());
        m_running = true;
        m_worker = std::thread(&FlTerminalEditor::WorkerThread, this);
    }

    ~FlTerminalEditor() 
    {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_running = false;
        }
        m_cv.notify_all();
        if (m_worker.joinable()) m_worker.join();
    }

    void RenderTerminal(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

private:
    friend class FlPythonMacroEditor;
    friend class FlDeveloperCommandPromptEditor;
    friend class FlScriptModuleEditor;

    std::string m_inputBuf;
    std::vector<std::string> m_log;
    bool m_scrollToBottom{ false };
    std::filesystem::path m_currentDir{ std::filesystem::current_path() };

    // 非同期処理用
    std::thread m_worker;
    std::atomic<bool> m_running{ false };
    std::queue<std::string> m_commandQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_isRunningCommand{ false };

    std::mutex m_processMutex;
    PROCESS_INFORMATION m_currentPI{}; // current child process info (zeroed when none)
    HANDLE m_hChildStdin_Wr = nullptr; // 親が書き込む側（子のstdin）
    std::atomic<bool> m_hasRunningProcess{ false };

    std::mutex m_logMutex;

    void ExecuteCommand(const char* cmd);
    void AddLog(const std::string& log);
    void WorkerThread();

    const auto& IsRunning() const noexcept { return m_isRunningCommand; }
};
