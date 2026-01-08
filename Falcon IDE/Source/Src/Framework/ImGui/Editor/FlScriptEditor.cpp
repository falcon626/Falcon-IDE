#include "FlScriptEditor.h"

FlScriptEditor::FlScriptEditor()
    : m_autoAdd("DynamicLib/FlComponentSystem/FlComponentSystem.vcxproj",
        "DynamicLib/FlComponentSystem/FlComponentSystem.vcxproj.filters")
{
    m_componentDir = "DynamicLib/FlComponentSystem/Src/ComponentSystem";
    RefreshFileList();
}

void FlScriptEditor::Render(const std::string& title, bool* p_open)
{
    if (ImGui::Begin(title.c_str(), p_open))
    {
        if (ImGui::Button("Refresh"))
        {
            RefreshFileList();
        }

        ImGui::Separator();
        DrawFileList();

        const bool isAnyItemHovered = ImGui::IsAnyItemHovered();
        const bool isRightClickReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
        const bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        if (isWindowHovered && isRightClickReleased && !isAnyItemHovered)
        {
            ImGui::OpenPopup("ScriptContextMenu");
        }

        if (ImGui::BeginPopup("ScriptContextMenu"))
        {
            if (ImGui::MenuItem("Create Script"))
            {
                m_pendingPopup = PopupRequest{ PopupRequest::Type::Create };
            }
            ImGui::EndPopup();
        }

        // ---- 個別ポップアップ（Create / Delete） ----
        RenderPopup();
    }
    ImGui::End();
}

void FlScriptEditor::RenderPopup()
{
    if (!m_pendingPopup) return;

    switch (m_pendingPopup->type)
    {
    case PopupRequest::Type::Create:
        ImGui::OpenPopup("CreateScriptPopup");
        break;

    case PopupRequest::Type::Delete:
        ImGui::OpenPopup("DeleteScriptPopup");
        break;
    }

    // ▼ スクリプト作成ポップアップ
    if (ImGui::BeginPopup("CreateScriptPopup"))
    {
        ImGui::InputText("Class Name", &m_newClassName);
        ImGui::InputText("Filter Name", &m_filterName);

        if (ImGui::Button("Create") && !m_newClassName.empty())
        {
            if (m_autoAdd.CreateAndAdd(m_newClassName, m_filterName))
            {
                FlEditorAdministrator::Instance().GetLogger()->AddLog(
                    "Script created: %s", m_newClassName.c_str());
                RefreshFileList();
            }
            else
            {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
                    "Failed to create script: %s", m_newClassName.c_str());
            }

            m_newClassName.clear();
            m_pendingPopup.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_newClassName.clear();
            m_pendingPopup.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ▼ 削除確認ポップアップ
    if (ImGui::BeginPopup("DeleteScriptPopup"))
    {
        ImGui::Text("Are you sure you want to delete this script?");
        ImGui::TextWrapped("%s", m_targetDeleteFile.filename().stem().string().c_str());

        if (ImGui::Button("Delete"))
        {
            DeleteScript(m_targetDeleteFile);
            m_pendingPopup.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_pendingPopup.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void FlScriptEditor::RefreshFileList()
{
    m_files.clear();
    if (!std::filesystem::exists(m_componentDir)) return;

    for (auto& entry : std::filesystem::directory_iterator(m_componentDir))
    {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext == ".h" || ext == ".cpp")
        {
            m_files.push_back(entry.path());
        }
    }
}

void FlScriptEditor::DrawFileList()
{
    const int columns = Def::BitMaskPos3;
    const ImVec2 iconSize(48, 48);

    ImGui::Columns(columns, nullptr, false);
    int id = 0;

    for (auto& path : m_files)
    {
        ImGui::PushID(id++);

        auto filename = path.filename().string();
        const char* icon = ".?";
        auto ext = path.extension().string();
        if (ext == ".h")   icon = ".h";
        if (ext == ".cpp") icon = ".cpp";

        ImGui::Button(icon, iconSize);

        // ダブルクリックで開く
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }

        // 右クリックメニュー
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Open"))
            {
                ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }

            if (ImGui::MenuItem("Delete"))
            {
                m_targetDeleteFile = path;
                m_pendingPopup = PopupRequest{ PopupRequest::Type::Delete };
            }

            ImGui::EndPopup();
        }

        ImGui::TextWrapped("%s", filename.c_str());
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
}

void FlScriptEditor::DeleteScript(const std::filesystem::path& path)
{
    try
    {
        std::string stem = path.stem().string();

        auto cppPath = m_componentDir / (stem + ".cpp");
        auto hPath = m_componentDir / (stem + ".h");

        bool deleted = false;

        if (std::filesystem::exists(cppPath))
        {
            std::filesystem::remove(cppPath);
            deleted = true;
        }
        if (std::filesystem::exists(hPath))
        {
            std::filesystem::remove(hPath);
            deleted = true;
        }

        if (deleted)
        {
            m_autoAdd.RemoveFile(stem, m_filterName);
            FlEditorAdministrator::Instance().GetLogger()->AddLog(
                "Deleted script: %s", stem.c_str());
            RefreshFileList();
        }
        else
        {
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
                "Failed to delete script (not found): %s", stem.c_str());
        }
    }
    catch (std::exception& e)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
            "Exception deleting script: %s", e.what());
    }
}
