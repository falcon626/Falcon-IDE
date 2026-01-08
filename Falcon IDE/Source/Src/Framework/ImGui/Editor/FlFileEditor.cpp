#include "FlFileEditor.h"

FlFileEditor::FlFileEditor() noexcept
{
    m_upFPM = std::make_unique<FlFilePathManager>();
}

void FlFileEditor::ShowAssetBrowser(const std::string& title, bool* p_open, ImGuiWindowFlags flags) noexcept
{
    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        if (ImGui::BeginTabBar("ControlTabs"))
        {
            // <Begin:Tab>
            if (ImGui::BeginTabItem("Current"))
            {
                RenderCurrentItem();

                ImGui::EndTabItem();
            }
            // <End:Tab>

            // <Begin:Tab>
            if (ImGui::BeginTabItem("Tree"))
            {
                auto root{ FileEntry{} };

                BuildFileTree(m_basePath, root);

                RenderFileEntry(root);

                ImGui::EndTabItem();
            }
            // <End:Tab>

            ImGui::EndTabBar();
        }

        if (!m_doppedPath.empty())
        {
            auto entryPath{ m_currentPath };

            auto dst{ entryPath / m_doppedPath.filename() };
            if (std::filesystem::exists(dst))
            {
                auto count{ Def::IntZero };
                auto stem{ dst.stem().string() };
                auto ext{ dst.extension().string() };
                do {
                    dst = entryPath / (stem + "_copy" + std::to_string(++count) + ext);
                } while (std::filesystem::exists(dst));
            }

            // ログ出力
            if (m_fileWatcher.Copy(m_doppedPath, dst))
            {
                auto dopp{ m_upFPM->GetRelative(m_doppedPath) };
                FlEditorAdministrator::Instance().GetLogger()->AddChangeLog("Dropped External File: %s to %s", dopp.string().c_str(), dst.string().c_str());
            }
            else
            {
                auto dopp{ m_upFPM->GetRelative(m_doppedPath) };
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Dropped External File: %s to %s", dopp.string().c_str(), dst.string().c_str());
            }

            m_doppedPath.clear();
        }

        RenderPopup();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
            !ImGui::IsAnyItemHovered())
        {
            ImGui::OpenPopup("BlankContextMenu");
        }

        if (ImGui::BeginPopup("BlankContextMenu"))
        {
            // <Begin:Create Menu>
            if (ImGui::MenuItem("Create")) 
                m_pendingPopup = PopupRequest{ m_currentPath, PopupRequest::Type::Create };
            // <End:Create Menu>

            // <Begin:Paste Menu>
            if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(ImGui::GetClipboardText())))
            {
                const auto text{ std::string{ImGui::GetClipboardText()} };
                auto src       { std::filesystem::path{} };
                auto isSafety  { true };

                try {
                    src = std::filesystem::path(text);
                }
                catch (const std::filesystem::filesystem_error& e) {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: %s", e.what());
                    isSafety = false;
                }
                catch (const std::exception& e) {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: %s", e.what());
                    isSafety = false;
                }

                if (isSafety)
                {
                    if (!src.is_absolute()) src = m_basePath / src;

                    auto dst{ m_currentPath / src.filename() };
                    auto count{ Def::IntZero };
                    while (std::filesystem::exists(dst)) {
                        dst = m_currentPath / (src.stem().string() + "_copy" + std::to_string(++count) + src.extension().string());
                    }

                    if (m_fileWatcher.Copy(src, dst))
                        FlEditorAdministrator::Instance().GetLogger()->AddLog("Paste: %s to %s", src.string().c_str(), dst.string().c_str());
                    else
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Paste %s to %s", src.string().c_str(), dst.string().c_str());
                }
            }
            // <End:Paste Menu>

            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void FlFileEditor::BuildFileTree(const std::filesystem::path& root, FileEntry& entry) noexcept
{
    entry.path = root;
    entry.isDirectory = std::filesystem::is_directory(root);

    if (entry.isDirectory) 
    {
        entry.children.clear();
        for (auto& p : std::filesystem::directory_iterator(root)) {
            auto child{ FileEntry{} };
            BuildFileTree(p, child);
            entry.children.push_back(child);
        }
    }
    else entry.children.clear();
}

void FlFileEditor::RenderFileEntry(FileEntry& entry) noexcept
{
    auto flags{ ImGuiTreeNodeFlags{entry.isDirectory ? ImGuiTreeNodeFlags_DrawLinesToNodes : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_DrawLinesToNodes} };

    auto label{ entry.path.filename().string() };
    if (label.empty()) label = "##Unnamed_" + entry.path.string();

    auto open{ false };
    if (entry.isDirectory) open = ImGui::TreeNodeEx(label.c_str(), flags | ImGuiTreeNodeFlags_DefaultOpen);
    else ImGui::TreeNodeEx(label.c_str(), flags | ImGuiTreeNodeFlags_NoTreePushOnOpen);

    // 右クリックで操作メニュー
    if (ImGui::BeginPopupContextItem()) 
    {
        // <Begin:Open Menu>
        if (ImGui::MenuItem("Open")) 
        {
            ShellExecuteW(nullptr, L"open", L"explorer.exe", entry.path.wstring().c_str(), nullptr, SW_SHOWNORMAL);

            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Open %s",
                entry.path.string().c_str());
        }
        // <End:Open Menu>

        // <Begin:Show In Explorer Menu>
        if (ImGui::MenuItem("Show In Explorer"))
        {
            if (!entry.isDirectory) ShellExecuteW(nullptr, L"open", L"explorer.exe", 
                entry.path.parent_path().wstring().c_str(), nullptr, SW_SHOWNORMAL);

            else ShellExecuteW(nullptr, L"open", L"explorer.exe", 
                entry.path.wstring().c_str(), nullptr, SW_SHOWNORMAL);

            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Show In Explorer %s",
                entry.path.string().c_str());
        }
        // <End:Show In Explorer Menu>

        // <Begin:Rename Menu>
        if (ImGui::MenuItem("Rename")) 
        {
            m_pendingPopup = PopupRequest{ entry.path, PopupRequest::Type::Rename };
        }
        // <End:Rename Menu>

        // <Begin:Create Menu>
        if (ImGui::MenuItem("Create")) 
        {
            m_pendingPopup = PopupRequest{ entry.path, PopupRequest::Type::Create };
        }
        // <End:Create Menu>

        // <Begin:Copy Guid Menu>
        if (ImGui::MenuItem("Copy Guid"))
        {
            auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(entry.path) };

            if (!guid.has_value())
                // ログ出力
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed Copy Guid by %s",
                    entry.path.string().c_str());
            else
            {
                ImGui::SetClipboardText(guid.value().c_str());

                // ログ出力
                FlEditorAdministrator::Instance().GetLogger()->AddLog("Copy Guid by %s",
                    entry.path.string().c_str());
            }
        }
        // <End:Copy Guid Menu>

        // <Begin:Copy Menu>
        if (ImGui::MenuItem("Copy")) 
        {
            const auto relative{ std::filesystem::relative(entry.path, m_basePath) };
            ImGui::SetClipboardText(relative.string().c_str());

            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Copy %s",
                entry.path.string().c_str());
        }
        // <End:Copy Menu>

        // <Begin:Paste Menu>
        if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(ImGui::GetClipboardText())))
        {
            if (entry.isDirectory)
            {
                const auto text{ std::string{ImGui::GetClipboardText()} };
                auto src{ std::filesystem::path{} };

                try {
                    src = std::filesystem::path(text);
                }
                catch (const std::exception& e) {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: %s", e.what());
                }

                if (src == entry.path || entry.path.string().find(src.string()) == NULL) 
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
                        "Error: Cannot paste into itself: %s", text.c_str());
                }
                else if (!std::filesystem::exists(src)) {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
                        "Error: Invalid path: %s", text.c_str());
                }       
                else
                {
                    auto dst{ std::filesystem::path{entry.path / src.filename()} };

                    auto count{ Def::IntZero };
                    while (std::filesystem::exists(dst)) {
                        dst = entry.path / (src.stem().string() + "_copy" + std::to_string(++count) + src.extension().string());
                    }

                    // ログ出力
                    if (m_fileWatcher.Copy(src, dst))
                        FlEditorAdministrator::Instance().GetLogger()->AddLog("Paste: %s to %s",
                            src.string().c_str(), dst.string().c_str());
                    else
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Paste %s to %s",
                            src.string().c_str(), dst.string().c_str());
                }
            }
        }
        // <End:Paste Menu>

        // <Begin:Delete Menu>
        if (ImGui::MenuItem("Delete")) 
        {
            m_pendingPopup = PopupRequest{ entry.path, PopupRequest::Type::Delete };
        }
        // <End:Delete Menu>
        
        ImGui::EndPopup();
    }

    // D&D
    if (ImGui::BeginDragDropSource()) 
    {
        ImGui::SetDragDropPayload("FILE_ENTRY_PATH", entry.path.string().c_str(), entry.path.string().size() + 1);
        ImGui::Text("%s", entry.path.filename().string().c_str()); // ゴースト表示
        ImGui::EndDragDropSource();
    }

    // ドロップターゲット
    if (entry.isDirectory && ImGui::BeginDragDropTarget()) 
    {
        if (const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload("FILE_ENTRY_PATH") })
        {
            auto srcPathStr{ std::string{static_cast<const char*>(payload->Data)} };
            auto srcPath{ std::filesystem::path{srcPathStr} };

            payload = nullptr;

            // 同じ場所への移動は無視
            if (srcPath != entry.path && srcPath.parent_path() != entry.path) 
            {
                auto dstPath{ std::filesystem::path{entry.path / srcPath.filename()} };
                if (m_fileWatcher.Move(srcPath, dstPath)) 
                {
                    auto count{ Def::IntZero };
                    while (std::filesystem::exists(dstPath)) {
                        dstPath = entry.path / (srcPath.stem().string() + "_move" + std::to_string(++count) + srcPath.extension().string());
                    }

                    FlResourceAdministrator::Instance().GetMetaFileManager()->OnAssetRenamedOrMoved(srcPath, dstPath);

                    // ログ出力
					FlEditorAdministrator::Instance().GetLogger()->AddLog("Moved: %s to %s", 
                        srcPath.string().c_str(), dstPath.string().c_str());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // 再帰描画
    if (entry.isDirectory && open) 
    {
        for (auto& child : entry.children) {
            RenderFileEntry(child);
        }
        ImGui::TreePop();
    }

}

void FlFileEditor::RenderPopup() noexcept
{
    if (!m_pendingPopup) return;

    auto& path{ m_pendingPopup->target };
    auto& state{ m_uiStates[path] };
    
    switch (m_pendingPopup->type)
    {
    case PopupRequest::Type::Rename:
        ImGui::OpenPopup("RenamePopup");
        break;
    case PopupRequest::Type::Create:
        ImGui::OpenPopup("CreatePopup");
        break;
    case PopupRequest::Type::Delete:
        ImGui::OpenPopup("DeletePopup");
        break;
    }
    
    // <Begin:RenamePopup>
    if (ImGui::BeginPopup("RenamePopup"))
    {
        if (state.isRenamePopupNewlyOpened) 
        {
            state.renameName = path.filename().string();
            state.isRenamePopupNewlyOpened = false;
        }

        ImGui::InputText("New Name", &state.renameName);
        if (ImGui::Button("Rename") && !state.renameName.empty())
        {
            auto newPath{ path.parent_path() / state.renameName };
            m_fileWatcher.RenameFile(path, newPath);
            state.renameName.clear();
            ImGui::CloseCurrentPopup();
            m_pendingPopup.reset();

            FlResourceAdministrator::Instance().GetMetaFileManager()->OnAssetRenamedOrMoved(path, newPath);

			// ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Renamed: %s to %s", 
				path.string().c_str(), newPath.string().c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            state.renameName.clear();
            ImGui::CloseCurrentPopup();
            m_pendingPopup.reset();
        }
        ImGui::EndPopup();
    }
    // <End:RenamePopup>
    
    // <Begin:CreatePopup>
    if (ImGui::BeginPopup("CreatePopup"))
    {
        auto mode{ state.createAsDirectory ? FALSE : TRUE };
    
        ImGui::RadioButton("Directory", &mode, FALSE);
        ImGui::SameLine();
        ImGui::RadioButton("File", &mode, TRUE);
        state.createAsDirectory = (mode == FALSE);
    
        ImGui::InputText("Name", &state.newName);
    
        if (ImGui::Button("Create") && !state.newName.empty())
        {
            auto newPath{ path / state.newName };
            if (state.createAsDirectory)
                m_fileWatcher.AddDirectory(newPath);
            else
                m_fileWatcher.AddFile(newPath, "");
            state.newName.clear();
            ImGui::CloseCurrentPopup();
            m_pendingPopup.reset();

			// ログ出力
			FlEditorAdministrator::Instance().GetLogger()->AddLog("Created: %s",
				newPath.string().c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            state.newName.clear();
            ImGui::CloseCurrentPopup();
            m_pendingPopup.reset();
        }
        ImGui::EndPopup();
    }
    // <End:CreatePopup>
   
    // <Begin:DeletePopup>
    if (ImGui::BeginPopup("DeletePopup"))
    {
        ImGui::Text("Are you sure you want to delete %s?", path.filename().string().c_str());
        if (ImGui::Button("Delete"))
        {
            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Deleted: %s",
                path.string().c_str());

            if (std::filesystem::is_directory(path))  m_fileWatcher.RemDirectory(path);
            else m_fileWatcher.RemFile(path);

            ImGui::CloseCurrentPopup();
            m_pendingPopup.reset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
            m_pendingPopup.reset();
        }
        ImGui::EndPopup();
    }
    // <End:DeletePopup>
}

void FlFileEditor::RenderFileItem(const std::filesystem::directory_entry& entry) noexcept
{
    const auto& path{ entry.path() };
    auto label      { path.filename().string() };

    if (label.empty()) label = "Unnamed";

    auto id{ label + "##" + path.string() };

    const auto isDir{ entry.is_directory() };

    auto displayLabel{ isDir ? "[Directory] " + label : "[File]" + label };
    if (ImGui::Selectable(displayLabel.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
    {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) 
        {
            if (isDir)  m_currentPath = path;
            else ShellExecuteW(nullptr, L"open", L"explorer.exe", path.wstring().c_str(), nullptr, SW_SHOWNORMAL);
        }
    }

    // D&D
    if (ImGui::BeginDragDropSource())
    {
        auto fullPathStr{ path.string() };
        ImGui::SetDragDropPayload("FILE_ENTRY_PATH", fullPathStr.c_str(), fullPathStr.size() + Def::ULongLongOne);
        ImGui::Text("%s", path.filename().string().c_str()); // ゴースト表示
        ImGui::EndDragDropSource();
    }

    // ドロップターゲット
    if (isDir && ImGui::BeginDragDropTarget()) 
    {
        if (const auto payload{ ImGui::AcceptDragDropPayload("FILE_ENTRY_PATH") }) 
        {
            auto srcStr{ std::string{static_cast<const char*>(payload->Data)} };
            auto srcPath{ std::filesystem::path{srcStr} };

            if (srcPath != path && srcPath.parent_path() != path) 
            {
                auto dst{ path / srcPath.filename() };

                auto count{ Def::UIntZero };
                while (std::filesystem::exists(dst)) {
                    dst = path / (srcPath.stem().string() + "_copy" + std::to_string(++count) + srcPath.extension().string());
                }

                if (m_fileWatcher.Move(srcPath, dst)) 
                {
                    FlResourceAdministrator::Instance().GetMetaFileManager()->OnAssetRenamedOrMoved(srcPath, dst);
                    // ログ出力
                    FlEditorAdministrator::Instance().GetLogger()->AddLog("Dropped: %s to %s",
                        srcPath.string().c_str(), dst.string().c_str());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // <Begin:Popup>
    if (ImGui::BeginPopupContextItem(id.c_str())) 
    {
        // <Begin:Open Menu>
        if (ImGui::MenuItem("Open"))
        {
            ShellExecuteW(nullptr, L"open", L"explorer.exe", path.wstring().c_str(), nullptr, SW_SHOWNORMAL);

            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Open %s",
                path.string().c_str());
        }
        // <End:Open Menu>
        
        // <Begin:Show In Explorer Menu>
        if (ImGui::MenuItem("Show In Explorer"))
        {
            if (!isDir) ShellExecuteW(nullptr, L"open", L"explorer.exe",
                path.parent_path().wstring().c_str(), nullptr, SW_SHOWNORMAL);

            else ShellExecuteW(nullptr, L"open", L"explorer.exe",
                path.wstring().c_str(), nullptr, SW_SHOWNORMAL);

            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Show In Explorer %s",
                path.string().c_str());
        }
        // <End:Show In Explorer Menu>
        
        // <Begin:Rename Menu>
        if (ImGui::MenuItem("Rename")) m_pendingPopup = PopupRequest{ path, PopupRequest::Type::Rename };
        // <End:Rename Menu>
        
        // <Begin:Create Menu>
        if (ImGui::MenuItem("Create")) m_pendingPopup = PopupRequest{ path, PopupRequest::Type::Create };
        // <End:Create Menu>
        
        // <Begin:Copy Guid Menu>
        if (ImGui::MenuItem("Copy Guid"))
        {
            auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(path) };

            if (!guid.has_value())
                // ログ出力
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed Copy Guid by %s",
                    path.string().c_str());
            else
            {
                ImGui::SetClipboardText(guid.value().c_str());

                // ログ出力
                FlEditorAdministrator::Instance().GetLogger()->AddLog("Copy Guid by %s",
                    path.string().c_str());
            }
        }
        // <End:Copy Guid Menu>

        // <Begin:Copy Menu>
        if (ImGui::MenuItem("Copy"))
        {
            const auto relative{ std::filesystem::relative(path, m_basePath) };
            ImGui::SetClipboardText(relative.string().c_str());

            // ログ出力
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Copy %s",
                path.string().c_str());
        }
        // <End:Copy Menu>
        
        // <Begin:Paste Menu>
        if (ImGui::MenuItem("Paste", nullptr, false, static_cast<bool>(ImGui::GetClipboardText())))
        {
            const auto text{ std::string{ImGui::GetClipboardText()} };
            auto src{ std::filesystem::path{} };
            auto isSafety{ true };

            try {
                src = std::filesystem::path(text);
            }
            catch (const std::filesystem::filesystem_error& e) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: %s", e.what());
                isSafety = false;
            }
            catch (const std::exception& e) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: %s", e.what());
                isSafety = false;
            }
            if (isSafety)
            {
                if (!src.is_absolute()) src = m_basePath / src;

                auto pasteTargetDir{ isDir ? path : path.parent_path() };

                // ログ出力
                if (std::filesystem::equivalent(src, pasteTargetDir))
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Cannot paste into itself: %s",
                        text.c_str());

                else if (!std::filesystem::exists(src))
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Invalid path: %s",
                        text.c_str());

                else
                {
                    auto dst{ pasteTargetDir / src.filename() };
                    auto count{ Def::IntZero };
                    while (std::filesystem::exists(dst)) {
                        dst = pasteTargetDir / (src.stem().string() + "_copy" + std::to_string(++count) + src.extension().string());
                    }

                    // ログ出力
                    if (m_fileWatcher.Copy(src, dst))
                        FlEditorAdministrator::Instance().GetLogger()->AddLog("Paste: %s to %s",
                            src.string().c_str(), dst.string().c_str());
                    else
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Paste %s to %s",
                            src.string().c_str(), pasteTargetDir.string().c_str());
                }
            }
        }
        // <End:Paste Menu>
        
        // <Begin:Delete Menu>
        if (ImGui::MenuItem("Delete")) m_pendingPopup = PopupRequest{ path, PopupRequest::Type::Delete };
        // <End:Delete Menu>

        ImGui::EndPopup();
    }
}

void FlFileEditor::RenderCurrentItem()
{
    ImGui::PushID("CurrentPathDrop");

    // 表示ラベル
    const auto label = "Current Path: " + m_currentPath.string();

    // パンくず
    if (ImGui::Selectable(label.c_str(), false))
        if (m_currentPath != m_basePath) m_currentPath = m_currentPath.parent_path();

    // ドロップターゲット
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_ENTRY_PATH"))
        {
            auto srcStr = std::string(static_cast<const char*>(payload->Data));
            auto srcPath = std::filesystem::path(srcStr);

            // 上層フォルダの決定
            auto dstDir = m_currentPath.parent_path();

            // basePathより上には移動禁止
            if (dstDir.string().find(m_basePath.string()) != 0)
            {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
                    "Error: Cannot move outside of base path: %s", dstDir.string().c_str());
            }
            else
            {
                auto dst = dstDir / srcPath.filename();

                // 重複回避
                int count = 0;
                while (std::filesystem::exists(dst)) {
                    dst = dstDir / (srcPath.stem().string() + "_move" + std::to_string(++count) + srcPath.extension().string());
                }

                // 実行
                if (m_fileWatcher.Move(srcPath, dst))
                {
                    FlResourceAdministrator::Instance().GetMetaFileManager()->OnAssetRenamedOrMoved(srcPath, dst);
                    FlEditorAdministrator::Instance().GetLogger()->AddLog("Dropped: %s to %s",
                        srcPath.string().c_str(), dst.string().c_str());
                }
                else
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Dropped Error: %s to %s",
                        srcPath.string().c_str(), dst.string().c_str());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();
    ImGui::PopID();

    // --- 現在のディレクトリの中身を描画 ---
    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(m_currentPath)) {
            RenderFileItem(entry);
        }
    }
    catch (const std::exception& e)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: %s", e.what());
    }
}

