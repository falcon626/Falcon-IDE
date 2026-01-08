#pragma once
#include "../../System/XMLParser/FlAutomaticFileAddSystem.h"

class FlScriptEditor
{
public:
    FlScriptEditor();

    void Render(const std::string& title, bool* p_open = nullptr);

private:
    struct PopupRequest
    {
        enum class Type { Create, Delete } type;
    };

    FlAutomaticFileAddSystem m_autoAdd;
    std::optional<PopupRequest> m_pendingPopup;
    std::string m_newClassName;
    std::string m_filterName = "Src/ComponentSystem"; // デフォルトフィルタ

    std::filesystem::path m_componentDir;
    std::filesystem::path m_targetDeleteFile;
    std::vector<std::filesystem::path> m_files;

    void RenderPopup();
    void RefreshFileList();
    void DrawFileList();
    void DeleteScript(const std::filesystem::path& path);
};
