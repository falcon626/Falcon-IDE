#include "FlDeveloperCommandPromptEditor.h"

FlDeveloperCommandPromptEditor::FlDeveloperCommandPromptEditor(const std::filesystem::path& slnPath, FlTerminalEditor& terminal)
    : m_parser(slnPath)
    , m_terminal(terminal)
    , m_upTicker(std::make_unique<FlChronus::Ticker>(FlChronus::ms(500)))
{
    m_log.push_back("> Loaded Solution: " + slnPath.string());
}

void FlDeveloperCommandPromptEditor::Render(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        // プロジェクト選択
        if (ImGui::BeginCombo("Project",
            m_selectedProject >= 0 ? m_parser.GetProjects()[m_selectedProject].name.c_str() : "Select"))
        {
            for (int i = 0; i < m_parser.GetProjects().size(); i++) {
                bool selected = (m_selectedProject == i);
                if (ImGui::Selectable(m_parser.GetProjects()[i].name.c_str(), selected)) {
                    m_selectedProject = i;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // コンフィグ選択
        const char* configs[] = { "Debug", "Release" };
        ImGui::Combo("Configuration", &m_selectedConfig, configs, IM_ARRAYSIZE(configs));

        // プラットフォーム選択
        const char* platforms[] = { "Win32", "x64" };
        ImGui::Combo("Platform", &m_selectedPlatform, platforms, IM_ARRAYSIZE(platforms));

        // アクション選択
        const char* actions[] = { "Build", "Clean" };
        ImGui::Combo("Action", &m_selectedAction, actions, IM_ARRAYSIZE(actions));

        if (ImGui::Button("Execute")) ExecuteBuild();

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        ImGui::Separator();
        RenderSystemMonitor();
    }
    ImGui::End();
}

void FlDeveloperCommandPromptEditor::InitSystemMonitor() noexcept
{
    if (!m_cpuInitialized)
    {
        PdhOpenQuery(nullptr, NULL, &m_cpuQuery);
        PdhAddCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &m_cpuTotal);
        PdhCollectQueryData(m_cpuQuery);
        m_cpuInitialized = true;
    }
}

const double FlDeveloperCommandPromptEditor::GetCpuUsage() noexcept
{
    if (!m_cpuInitialized) InitSystemMonitor();

    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(m_cpuQuery);
    PdhGetFormattedCounterValue(m_cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal);
    return counterVal.doubleValue;
}

void FlDeveloperCommandPromptEditor::GetMemoryUsage(SIZE_T& workingSet, SIZE_T& peakWorkingSet) noexcept
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        workingSet = pmc.WorkingSetSize;
        peakWorkingSet = pmc.PeakWorkingSetSize;
    }
}

DWORD FlDeveloperCommandPromptEditor::GetThreadCount() noexcept
{
    DWORD currentPID = GetCurrentProcessId();
    DWORD threadCount = 0;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe32{};
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (pe32.th32ProcessID == currentPID)
                {
                    threadCount = pe32.cntThreads;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    return threadCount;
}

void FlDeveloperCommandPromptEditor::RenderSystemMonitor()
{
    InitSystemMonitor();

    m_cpuUsage = GetCpuUsage();
    GetMemoryUsage(m_workingSet, m_peakWorkingSet);

    if (m_upTicker->tick())
    {
        m_cpuUsageTick = m_cpuUsage;
        m_threadList = GetThreadList();
    }

    ImGui::Text("PID: %d", m_currentPID);

    ImGui::Text("CPU Usage: %.2f%%", m_cpuUsageTick);
    ImGui::ProgressBar(static_cast<float>(m_cpuUsageTick / 100.0), ImVec2(0.0f, 0.0f));

    ImGui::Separator();
    ImGui::Text("Memory Usage: %.2f MB", m_workingSet / (1024.0 * 1024.0));
    ImGui::Text("Peak Memory: %.2f MB", m_peakWorkingSet / (1024.0 * 1024.0));

    ImGui::Text("Total Threads: %d", static_cast<int>(m_threadList.size()));
    ImGui::Separator();

    if (ImGui::BeginTable("ThreadTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Thread ID");
        ImGui::TableSetupColumn("Priority");
        ImGui::TableSetupColumn("User Time (s)");
        ImGui::TableSetupColumn("Kernel Time (s)");
        ImGui::TableSetupColumn("CPU Total (s)");
        ImGui::TableHeadersRow();

        for (auto& t : m_threadList)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%lu", t.threadId);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", t.priority);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", t.cpuUserTimeSec);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.2f", t.cpuKernelTimeSec);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.2f", t.cpuUserTimeSec + t.cpuKernelTimeSec);
        }

        ImGui::EndTable();
    }
}

std::vector<FlDeveloperCommandPromptEditor::ThreadInfo> FlDeveloperCommandPromptEditor::GetThreadList()
{
    std::vector<ThreadInfo> threads;
    m_currentPID = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return threads;

    THREADENTRY32 te32{};
    te32.dwSize = sizeof(te32);

    if (Thread32First(hSnapshot, &te32))
    {
        do
        {
            if (te32.th32OwnerProcessID == m_currentPID)
            {
                ThreadInfo info{};
                info.threadId = te32.th32ThreadID;
                info.ownerProcessId = te32.th32OwnerProcessID;

                HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
                if (hThread)
                {
                    FILETIME createTime, exitTime, kernelTime, userTime;
                    if (GetThreadTimes(hThread, &createTime, &exitTime, &kernelTime, &userTime))
                    {
                        ULARGE_INTEGER k, u;
                        k.LowPart = kernelTime.dwLowDateTime;
                        k.HighPart = kernelTime.dwHighDateTime;
                        u.LowPart = userTime.dwLowDateTime;
                        u.HighPart = userTime.dwHighDateTime;

                        info.cpuKernelTimeSec = (double)k.QuadPart / 10000000.0;
                        info.cpuUserTimeSec = (double)u.QuadPart / 10000000.0;
                    }

                    info.priority = GetThreadPriority(hThread);
                    CloseHandle(hThread);
                }

                threads.push_back(info);
            }
        } while (Thread32Next(hSnapshot, &te32));
    }

    CloseHandle(hSnapshot);
    return threads;
}

void FlDeveloperCommandPromptEditor::ExecuteBuild()
{
    if (m_selectedProject < 0) {
        m_terminal.AddLog("No project selected.");
        return;
    }

    const auto& proj = m_parser.GetProjects()[m_selectedProject];

    std::string config = (m_selectedConfig == 0) ? "Debug" : "Release";
    std::string platform = (m_selectedPlatform == 0) ? "Win32" : "x64";
    std::string action = (m_selectedAction == 0) ? "Build" : "Clean";

    std::string solutionDir = std::filesystem::absolute(std::filesystem::current_path()).string() + "\\\\";

    std::string command =
        "call \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" " +
        platform + " && msbuild \"" + proj.fullPath.string() +
        "\" /p:Configuration=" + config +
        " /p:Platform=" + platform +
        " /p:SolutionDir=\"" + solutionDir + "\"" +
        " /t:" + action;

    m_terminal.ExecuteCommand(command.c_str());
}
