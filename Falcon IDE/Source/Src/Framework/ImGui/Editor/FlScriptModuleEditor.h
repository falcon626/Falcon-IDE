#pragma once
#include "../System/VisualStudioManager/FlVisualStudioManager.h"

class FlScriptModuleLoader;

class FlScriptModuleEditor
{
public:
    FlScriptModuleEditor(FlTerminalEditor& terminal);
    ~FlScriptModuleEditor();

    void Render(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

private:
    void ChangedFilesRefresh() noexcept;

    void Update() noexcept;

    void RenderPopup();
    bool CanDeleteProject(const std::string& projectName) const noexcept;

    std::unique_ptr<FlVisualStudioProjectManager> m_manager;
    std::unique_ptr<FlMetaFileManager> m_meta;
    std::unique_ptr<FlChronus::Ticker> m_upTicker;

    const std::filesystem::path m_targetDir{"DynamicLib/"};

    // Popupä«óù
    struct PopupRequest {
        enum class Type { Create, Delete } type;
        std::string targetProjectName;
    };

    std::optional<PopupRequest> m_pendingPopup;
    std::unique_ptr<FlScriptModuleLoader> m_loader;

    FlTerminalEditor& m_terminal;

    std::list<std::string> m_codeFiles;
    std::list<std::string> m_changerdCodeFiles;

    // ì¸óÕì‡óe
    std::string m_newProjectName;

    struct PendingBuild
    {
        std::future<bool> completion;
        std::filesystem::file_time_type sourceWriteTime;
    };
    std::unordered_map<std::string, PendingBuild> m_pendingBuilds;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_failedBuilds;
};
