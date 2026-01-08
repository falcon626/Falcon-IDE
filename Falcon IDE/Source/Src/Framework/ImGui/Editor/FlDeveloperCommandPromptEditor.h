#pragma once
#include "../../System/SolutionParser/FlSolutionParser.h"

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#include <psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tlhelp32.h>

class FlDeveloperCommandPromptEditor
{
public:
    explicit FlDeveloperCommandPromptEditor(const std::filesystem::path& slnPath, FlTerminalEditor& terminal);

    void Render(const std::string& title, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

private:
    void ExecuteBuild();

    // 初期化
    void InitSystemMonitor() noexcept;

    // CPU使用率取得
    const double GetCpuUsage() noexcept;

    // メモリ情報取得
    void GetMemoryUsage(SIZE_T& workingSet, SIZE_T& peakWorkingSet) noexcept;

    // スレッド数取得
    DWORD GetThreadCount() noexcept;

    // ImGuiウィンドウ描画
    void RenderSystemMonitor();

    struct ThreadInfo
    {
        DWORD threadId;
        DWORD ownerProcessId;
        int priority;
        double cpuUserTimeSec;
        double cpuKernelTimeSec;
    };

    std::vector<ThreadInfo> GetThreadList();

    FlSolutionParser m_parser{};

    int m_selectedProject = -1;
    int m_selectedConfig = 0;   // 0=Debug, 1=Release
    int m_selectedPlatform = 0; // 0=Win32, 1=x64
    int m_selectedAction = 0;   // 0=Build, 1=Clean

    std::vector<std::string> m_log;
    bool m_scrollToBottom{ false };

    FlTerminalEditor& m_terminal;

    PDH_HQUERY m_cpuQuery{};
    PDH_HCOUNTER m_cpuTotal{};
    bool m_cpuInitialized{ false };

    std::unique_ptr<FlChronus::Ticker> m_upTicker;

    std::vector<ThreadInfo> m_threadList;

    double m_cpuUsage = 0.0;
    double m_cpuUsageTick = 0.0;
    SIZE_T m_workingSet = 0;
    SIZE_T m_peakWorkingSet = 0;

    DWORD m_currentPID{};
};
