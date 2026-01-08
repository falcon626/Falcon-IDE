#include "FlInspectorAndHierarchyEditor.h"

void FlInspectorAndHierarchyEditor::RenderInspector(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        auto sp = m_selectedEntity.lock();
        if (sp)
        {
            // ---- Entity情報の表示と編集 ----
            ImGui::Text("ID: %d", sp->id); // IDは表示のみ
            ImGui::Separator();

            // 名前の編集用InputText
            char nameBuffer[Def::BitMaskPos9];
            strncpy_s(nameBuffer, sizeof(nameBuffer), sp->name.c_str(), sizeof(nameBuffer) - Def::ULongLongOne);
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                sp->name = nameBuffer;
            }

            ImGui::SeparatorText("Components");

            for (auto& comp : sp->components) {
                if (ImGui::CollapsingHeader(comp->GetType().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    // 各コンポーネントごとの ImGui 表示
                    comp->RenderImGui();

                    // Remove ボタン
                    if (ImGui::Button(("Remove##" + comp->GetType()).c_str())) // ボタンのIDをユニークにする
                    {
                        FlScene::Instance().DetachComponentLater(sp->id, comp->GetType());
                    }
                    ImGui::Unindent();
                }
            }

            ImGui::Separator();

            const auto& allTypes = FlScene::Instance().GetCSDLLController()->GetComponentTypes();
            std::vector<const char*> attachableTypes;
            for (const auto& typeName : allTypes) {
                // 既にアタッチ済みであるかどうか
                bool alreadyAttached = false;
                for (const auto& existingComp : sp->components) {
                    if (existingComp->GetType() == typeName)
                    {
                        alreadyAttached = true;
                        break;
                    }
                }
                if (!alreadyAttached) 
                    attachableTypes.push_back(typeName.c_str());
            }

            if (!attachableTypes.empty())
            {
                ImGui::Text("Add Component");
                static int selectedIndex = Def::IntZero;
                if (selectedIndex >= attachableTypes.size()) {
                    selectedIndex = Def::IntZero; // 範囲外アクセスを防ぐ
                }

                if (ImGui::BeginCombo("##ComponentTypes", attachableTypes[selectedIndex])) 
                {
                    for (auto i{ Def::IntZero }; i < attachableTypes.size(); ++i) {
                        bool isSelected = (i == selectedIndex);
                        if (ImGui::Selectable(attachableTypes[i], isSelected)) 
                            selectedIndex = i;
                        
                        if (isSelected) 
                            ImGui::SetItemDefaultFocus();
                        
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Attach")) 
                    FlScene::Instance().AttachComponent(sp->id, attachableTypes[selectedIndex]);
            }
        }
        else 
            ImGui::Text("No entity selected.");
    }
    ImGui::End();
}

void FlInspectorAndHierarchyEditor::RenderHierarchy(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    decltype(auto) scene{ FlScene::Instance() };

    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        if (ImGui::Button("Add Entity")) scene.CreateEntity(scene.GetRootEntity());
        if (ImGui::Button("Load Scene")) {
            std::string filepath = "Assets/Scene/";
            if (OpenFileDialog(filepath, "Load Scene", "Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0")) {
                scene.LodeScene(filepath);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Scene")) {
            std::string filepath = "Assets/Scene/";
            if (SaveFileDialog(filepath, "Save Scene", "Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0", "scene")) {
                scene.SaveScene(filepath);
            }
        }

        if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& child : scene.GetRootEntity()->children) {
                if (auto sp{ child.lock() }) RenderEntityRecursive(sp);
            }
            ImGui::TreePop();
        }
        if (ImGui::BeginPopupContextWindow("BackContextMenu", ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Entity"))
            {
                scene.CreateEntity(scene.GetRootEntity());
            }
            if (ImGui::MenuItem("Instantiate Prefab"))
            {
                std::string filepath = "Assets/Prefabs/"; // 初期ディレクトリ
                if (OpenFileDialog(filepath, "Instantiate Prefab", "Prefab Files (*.prefab)\0*.prefab\0All Files (*.*)\0*.*\0"))
                {
                    scene.GetCSDLLController()->InstantiatePrefab(filepath, scene.GetRootEntity());
                }
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void FlInspectorAndHierarchyEditor::RenderEntityRecursive(const std::weak_ptr<Entity>& entity)
{
    if (entity.expired()) return;

    auto spEnt = entity.lock();
    auto& scene = FlScene::Instance();

    // ツリーノードのフラグ設定
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (spEnt->children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf; // 子がいなければ葉ノード
    }
    if (m_selectedEntity.lock() == spEnt) {
        flags |= ImGuiTreeNodeFlags_Selected; // 選択状態を反映
    }

    // --- 名前変更処理 ---
    static int renamingId = -1;
    bool isRenaming = (renamingId == spEnt->id);

    // ダブルクリックで名前変更モードへ
    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
        isRenaming = true;
        renamingId = spEnt->id;
    }

    // 名前変更モードでないなら、通常のツリーノードを表示
    if (!isRenaming) 
    {
        bool nodeOpen = ImGui::TreeNodeEx(
            (void*)(intptr_t)spEnt->id,
            flags,
            "%s", spEnt->name.c_str()
        );

        // クリックで選択
        if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) {
            m_selectedEntity = spEnt;
        }

        // 右クリックメニューは「ノード自体の描画があるときのみ」表示
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("EntityContextMenu_" + std::to_string(spEnt->id)).c_str());
        }

        // --- 右クリックメニューの描画 ---
        if (ImGui::BeginPopup(("EntityContextMenu_" + std::to_string(spEnt->id)).c_str()))
        {
            if (ImGui::MenuItem("Create Child Entity")) scene.CreateEntity(spEnt);
            if (ImGui::MenuItem("Rename")) {
                isRenaming = true;
                renamingId = spEnt->id;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Create Prefab")) {
                std::string filepath = "Assets/Prefabs/" + spEnt->name + ".prefab";
                if (SaveFileDialog(filepath, "Create Prefab",
                    "Prefab Files (*.prefab)\0*.prefab\0All Files (*.*)\0*.*\0", "prefab"))
                {
                    scene.GetCSDLLController()->CreatePrefab(filepath, spEnt);
                }
            }

            if (ImGui::MenuItem("Instantiate Prefab")) {
                std::string filepath = "Assets/Prefabs/";
                if (OpenFileDialog(filepath, "Instantiate Prefab",
                    "Prefab Files (*.prefab)\0*.prefab\0All Files (*.*)\0*.*\0"))
                {
                    scene.GetCSDLLController()->InstantiatePrefab(filepath, spEnt);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete Entity", nullptr, false, spEnt->id != 0)) {
                scene.DeleteEntityLater(spEnt->id);
            }

            ImGui::EndPopup();
        }

    // Drag & Drop Source: ノードをドラッグ可能にする
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) 
    {
        ImGui::SetDragDropPayload("ENTITY_DRAG", &spEnt->id, sizeof(int)); // ペイロードにエンティティIDをセット
        ImGui::Text("Dragging Entity %d", spEnt->id);
        ImGui::EndDragDropSource();
    }

    // Drag & Drop Target: ノードをドロップターゲットにする
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) 
        {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int draggedId = *static_cast<int*>(payload->Data);
            auto draggedEntity = FlScene::Instance().GetEntityById(draggedId).lock();

            if (draggedEntity && draggedEntity != spEnt)
            {
                if (auto oldParent = draggedEntity->parent.lock()) 
                {
                    oldParent->children.erase(std::remove_if(oldParent->children.begin(), oldParent->children.end(),
                        [draggedId](const std::weak_ptr<Entity>& child) { return child.lock() && child.lock()->id == draggedId; }),
                        oldParent->children.end());
                }

                spEnt->children.emplace_back(draggedEntity);

                draggedEntity->parent = spEnt;

                for (auto& sys : FlScene::Instance().GetSystems()) {
                    auto supportedType{ FlScene::Instance().GetCSDLLController()->GetSupportedComponentType(sys)};
                    for (auto& component : spEnt->components) {
                        for (auto& compChi:draggedEntity->components) {
                            if ((component->GetType() == supportedType) && (compChi->GetType() == supportedType))
                                sys->AddChild(component, draggedEntity->id);
                        }
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

        auto spSelEnt{ m_selectedEntity.lock() };

        if (nodeOpen)
        {
            for (auto& child : spEnt->children) {
                RenderEntityRecursive(child);
            }
            ImGui::TreePop();
        }
    }

    // --- 名前変更モードのUI ---
    if (isRenaming) 
    {
        char buffer[Def::BitMaskPos9];
        strncpy_s(buffer, sizeof(buffer), spEnt->name.c_str(), sizeof(buffer) - 1);

        ImGui::SetKeyboardFocusHere(); // InputTextに自動でフォーカス
        if (ImGui::InputText("##Rename", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) 
        {
            spEnt->name = buffer;
            renamingId = -1; // 名前変更完了
        }
        if (ImGui::IsItemDeactivatedAfterEdit() || (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))) 
            renamingId = -1; // フォーカスが外れたら完了
    }
}

bool FlInspectorAndHierarchyEditor::OpenFileDialog(std::string& filepath, const std::string& title, const char* filters)
{
    auto current = std::filesystem::current_path();
    auto filename = std::filesystem::path(filepath).filename();

    static char fname[1000];
    strcpy_s(fname, sizeof(fname), filename.string().c_str());

    std::string dir;
    if (filepath.empty()) dir = current.string() + "\\";
    else 
    {
        auto path = std::filesystem::absolute(filepath);
        dir = path.parent_path().string() + "\\";
    }

    OPENFILENAMEA o{};
    o.lStructSize = sizeof(o);
    o.hwndOwner = nullptr;
    o.lpstrInitialDir = dir.c_str();
    o.lpstrFile = fname;
    o.nMaxFile = sizeof(fname);
    o.lpstrFilter = filters;
    o.lpstrDefExt = "";
    o.lpstrTitle = title.c_str();
    o.nFilterIndex = 1;

    if (GetOpenFileNameA(&o)) {
        std::filesystem::current_path(current);
        filepath = std::filesystem::relative(fname).string();
        return true;
    }
    std::filesystem::current_path(current);
    return false;
}

bool FlInspectorAndHierarchyEditor::SaveFileDialog(std::string& filepath, const std::string& title, const char* filters, const std::string& defExt)
{
    auto current = std::filesystem::current_path();
    auto filename = std::filesystem::path(filepath).filename();

    static char fname[1000];
    strcpy_s(fname, sizeof(fname), filename.string().c_str());

    std::string dir;
    if (filepath.empty()) dir = current.string() + "\\";
    else 
    {
        auto path = std::filesystem::absolute(filepath);
        dir = path.parent_path().string() + "\\";
    }

    OPENFILENAMEA o{};
    o.lStructSize = sizeof(o);
    o.hwndOwner = nullptr;
    o.lpstrInitialDir = dir.c_str();
    o.lpstrFile = fname;
    o.nMaxFile = sizeof(fname);
    o.lpstrFilter = filters;
    o.lpstrDefExt = defExt.c_str();
    o.lpstrTitle = title.c_str();
    o.nFilterIndex = 1;
    o.Flags = OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&o))
    {
        std::filesystem::current_path(current);
        filepath = std::filesystem::relative(fname).string();
        return true;
    }
    std::filesystem::current_path(current);
    return false;
}