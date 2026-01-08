#include "FlScriptModuleEditor.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

FlScriptModuleEditor::FlScriptModuleEditor(FlTerminalEditor& terminal)
    : m_manager{ std::make_unique<FlVisualStudioProjectManager>("FlProject-DX12.sln")}
    , m_meta{ std::make_unique<FlMetaFileManager>() }
    , m_upTicker{ std::make_unique<FlChronus::Ticker>(FlChronus::sec(Def::UIntOne)) }
    , m_terminal{ terminal }
{
    m_meta->StartMonitoring("ScriptModule");
    m_codeFiles = m_meta->GetAllFilePaths();
}

FlScriptModuleEditor::~FlScriptModuleEditor()
{
    m_meta->StopMonitoring();
}

void FlScriptModuleEditor::Render(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        if (ImGui::Button("Create New Project"))
        {
            m_pendingPopup = PopupRequest{ PopupRequest::Type::Create };
        }

        const int columns = Def::BitMaskPos3;
        const ImVec2 iconSize(48, 48);

        ImGui::Columns(columns, nullptr, false);
        int id = 0;

        for (auto& path : m_codeFiles)
        {
            ImGui::PushID(id++);

            auto isChanged = m_meta->IsAssetChanged(path);

            auto filename = std::filesystem::path(path).filename().string();
            auto folder = std::filesystem::path(path).parent_path().filename().string();
            const char* icon = "?";
            auto ext = Str::FileExtensionSearcher(path);
            if (ext == "cxx") icon = "SCC";

            // ★ 右クリックメニューの開始
            if (ImGui::BeginPopupContextItem("ProjectContext"))
            {
                if (ImGui::MenuItem("Delete Project"))
                {
                    // プロジェクト名（フォルダ名）を保存
                    m_pendingPopup = PopupRequest{ PopupRequest::Type::Delete, folder };
                }
                ImGui::EndPopup();
            }

            if (isChanged)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.7f, 0.40f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.8f, 0.50f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.85f, 0.50f, 1.0f));

                ImGui::Button(icon, iconSize);

                ImGui::PopStyleColor(3);
            }
            else ImGui::Button(icon, iconSize);

            // ダブルクリックで開く
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                ShellExecuteW(nullptr, L"open", ansi_to_wide(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL);

            // ★ 右クリックによるメニュー表示のため
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                ImGui::OpenPopup("ProjectContext");

            ImGui::TextWrapped("%s", filename.c_str());
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);

        RenderPopup();
    }
    ImGui::End();

    Update();
}

void FlScriptModuleEditor::ChangedFilesRefresh() noexcept
{
    m_codeFiles = m_meta->GetAllFilePaths();
    m_changerdCodeFiles.clear();

    for (auto& path : m_codeFiles)
        if(m_meta->IsAssetChanged(path))
            m_changerdCodeFiles.push_back(path);
}

void FlScriptModuleEditor::Update() noexcept
{
    if (!m_upTicker->tick()) return;
    ChangedFilesRefresh();

    for (auto& file : m_changerdCodeFiles)
    {
        auto projName{ std::filesystem::path(file).parent_path().filename() };
        m_manager->FormingModule(
            m_targetDir / projName,
            file);
 
        std::string solutionDir = std::filesystem::absolute(std::filesystem::current_path()).string() + "\\\\";
 
        auto projPath{ m_targetDir / projName / (projName.string() + ".vcxproj") };
 
        // vcvarsall.bat を呼び出してビルド環境を初期化
        std::string command =
            "call \"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x64 && msbuild \"" +
            std::filesystem::absolute(projPath).lexically_normal().string() +
            "\" /p:Configuration=Release"
            " /p:Platform=x64" +
            " /p:SolutionDir=\"" + solutionDir + "\"" +
            " /t:Build";
 
        m_isDirty = true;
 
        m_terminal.ExecuteCommand(command.c_str());
        //FlScene::Instance().GetScriptModuleLoader()->ScanModule(std::filesystem::path(file).parent_path().filename());
        m_meta->ResetAssetChangeFlag(file);
    }
 
}

void FlScriptModuleEditor::RenderPopup()
{
    if (!m_pendingPopup) return;

    // --- Create Popup ---
    if (m_pendingPopup->type == PopupRequest::Type::Create)
        ImGui::OpenPopup("CreateVSPopup");

    // --- Delete Popup ---
    if (m_pendingPopup->type == PopupRequest::Type::Delete)
        ImGui::OpenPopup("DeleteProjectPopup");

    // ▼ Create ポップアップ（既存）
    if (ImGui::BeginPopup("CreateVSPopup"))
    {
        ImGui::InputText("Project Name", &m_newProjectName);

        if (ImGui::Button("Create") && !m_newProjectName.empty())
        {
            auto ok{ m_manager->CreateNewProject(
                m_newProjectName,
                m_targetDir
            ) };

            if (ok)
            {
                FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog(
                    "Created new project: %s", m_newProjectName.c_str());
                m_codeFiles = m_meta->GetAllFilePaths();
                ChangedFilesRefresh();
            }
            else
            {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
                    "Failed to create project: %s", m_newProjectName.c_str());
            }

            m_newProjectName.clear();
            m_pendingPopup.reset();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_newProjectName.clear();
            m_pendingPopup.reset();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // ▼ Delete ポップアップ
    if (ImGui::BeginPopup("DeleteProjectPopup"))
    {
        auto& projName = m_pendingPopup->targetProjectName;

        ImGui::Text("Delete project \"%s\" ?", projName.c_str());
        ImGui::Separator();

        if (ImGui::Button("Delete"))
        {
            FlEntityComponentSystemKernel::Instance().ClearComponent(std::filesystem::path(projName).stem().string());

            auto dir{ m_targetDir / projName };
            auto dllPath{"Src/Framework/Module/ScriptDLLs/" + projName};
            auto upFile{ std::make_unique<FlFileWatcher>() };
            
            if(!upFile->RemDirectory(dllPath))
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
					"Failed to delete DLL directory: %s", dllPath.c_str());
            if(!upFile->RemDirectory(dir))
				FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
					"Failed to delete project directory: %s", dir.c_str());

			if (!m_manager->RemoveProjectFromSolution((m_targetDir / projName / (projName + ".vcxproj")).lexically_normal()))
				FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
					"Failed to delete sln to project or scriptCode fi directory: %s", projName.c_str());

            FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog(
                "Deleted project: %s", projName.c_str());

            m_codeFiles = m_meta->GetAllFilePaths();

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
