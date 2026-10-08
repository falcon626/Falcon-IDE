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
            while (!m_commandQueue.empty())
            {
                m_commandQueue.front().completion.set_value(false);
                m_commandQueue.pop();
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_processMutex);
            if (m_processJob) CloseHandle(m_processJob);
            m_processJob = nullptr;
        }
        if (m_worker.joinable()) CancelSynchronousIo(m_worker.native_handle());
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
    std::atomic<bool> m_scrollToBottom{ false };
    std::filesystem::path m_currentDir{ std::filesystem::current_path() };

    // îÒìØä˙èàóùóp
    std::thread m_worker;
    std::atomic<bool> m_running{ false };
    struct CommandJob
    {
        std::string command;
        std::promise<bool> completion;
    };
    std::queue<CommandJob> m_commandQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_isRunningCommand{ false };

    std::mutex m_processMutex;
    HANDLE m_processJob = nullptr; // Owns only processes launched by this terminal.

    std::mutex m_logMutex;

    std::future<bool> ExecuteCommand(const char* cmd);
    void AddLog(const std::string& log);
    void WorkerThread();

    const auto& IsRunning() const noexcept { return m_isRunningCommand; }
};
