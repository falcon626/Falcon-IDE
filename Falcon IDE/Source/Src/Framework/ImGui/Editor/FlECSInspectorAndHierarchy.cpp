#include "FlECSInspectorAndHierarchy.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

#include "../../Framework/Module/FlRuntimeModuleGroup.hpp"

FlECSInspectorAndHierarchy::FlECSInspectorAndHierarchy()
{
    RefreshEntityList();
}

void FlECSInspectorAndHierarchy::Render(const char* hierarchyTitle, const char* inspectorTitle, bool* p_openHierarchy, bool* p_openInspector)
{
    RenderHierarchyWindow(hierarchyTitle, p_openHierarchy);
    RenderInspectorWindow(inspectorTitle, p_openInspector);
}

void FlECSInspectorAndHierarchy::RefreshEntityList()
{
    m_entityList.clear();
    auto ids = FlEntityComponentSystemKernel::Instance().GetAllEntityIds();
    // Sort for stable order
    std::sort(ids.begin(), ids.end());
    m_entityList = std::move(ids);
}

std::vector<uint32_t> FlECSInspectorAndHierarchy::GetRootEntities()
{
    std::vector<uint32_t> roots;

    for (auto id : m_entityList)
    {
        auto tc{ static_cast<TransformComponent*>(
            FlEntityComponentSystemKernel::Instance().GetComponent("Transform", id)
            ) };

        if (!tc) continue;

        if (tc->m_parent == UINT32_MAX)
            roots.push_back(id);
    }
    return roots;
}

static void CollectEntitiesRecursive(
	FlEntityComponentSystemKernel& kernel,
	uint32_t root,
	std::vector<uint32_t>& out)
{
	out.push_back(root);

	if (auto tc = static_cast<TransformComponent*>(
		kernel.GetComponent("Transform", root)))
	{
		for (auto c : tc->m_children)
			CollectEntitiesRecursive(kernel, c, out);
	}
}

// ---------- Hierarchy ----------
void FlECSInspectorAndHierarchy::RenderHierarchyWindow(const char* title, bool* p_open)
{
    if (!ImGui::Begin(title, p_open))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Save Scene"))
    {
        std::string filepath{ "Assets/Scene/" };
        if (SaveFileDialog(filepath, "Save Scene", "Scene Files (*.flscene)\0*.flscene\0All Files (*.*)\0*.*\0", "flscene"))
            FlJsonUtility::Serialize(FlEntityComponentSystemKernel::Instance().SerializeScene(), filepath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene"))
    {
        std::string filepath{ "Assets/Scene/" };
        if (OpenFileDialog(filepath, "Load Scene", "Scene Files (*.flscene)\0*.flscene\0All Files (*.*)\0*.*\0"))
        {
            auto j{ nlohmann::json{} };
            if (FlJsonUtility::Deserialize(j, filepath))
            {
                FlEntityComponentSystemKernel::Instance().DeserializeScene(j);
                RefreshEntityList();
            }
            else
                FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed load scene %s", filepath.c_str());
        }
    }

    if (ImGui::Button("Refresh"))
        RefreshEntityList();

    ImGui::SameLine();
    if (ImGui::Button("Create Entity"))
    {
        FlEntityComponentSystemKernel::Instance().CreateEntity();
        RefreshEntityList();
    }

    ImGui::Separator();

    // ---- ROOT ノードだけ描画 ----
    for (auto rootId : GetRootEntities())
        RenderEntityNode(rootId);

    ImVec2 availableSpace = ImGui::GetContentRegionAvail();

    if (availableSpace.y > 0.0f)
    {
        ImGui::InvisibleButton("DropTargetSpace", availableSpace);

        if (ImGui::BeginDragDropTarget())
        {
            if (auto payload = ImGui::AcceptDragDropPayload("ENTITY_ID"))
            {
                uint32_t childId = *(uint32_t*)payload->Data;
                SetParent(childId, UINT32_MAX);
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
}

// ---------- Inspector ----------
void FlECSInspectorAndHierarchy::RenderInspectorWindow(const char* title, bool* p_open)
{
	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	if (m_selectedEntityId == UINT32_MAX) {
		ImGui::TextUnformatted("No entity selected.");
		if (ImGui::Button("Refresh Entities")) RefreshEntityList();
		ImGui::End();
		return;
	}

	entityId id{ m_selectedEntityId };
	ImGui::Text("Entity ID: %u", id);
	ImGui::Separator();

	// --- Components attached to this entity ---
	auto compTypes = FlEntityComponentSystemKernel::Instance().GetEntityComponentTypes(id);
	ImGui::Text("Components (%zu)", compTypes.size());
	ImGui::Separator();

	for (const auto& typeName : compTypes)
	{
		// Collapsing header per component type
		if (ImGui::CollapsingHeader(typeName.c_str()))
		{
			// Try to call kernel's RenderComponentEditor which will call reflection.RenderEditor
			bool rendered = FlEntityComponentSystemKernel::Instance().RenderComponentEditor(typeName, id);
			if (!rendered) {
				ImGui::TextDisabled("No editor for this component (or missing reflection).");
			}

			if (typeName == "Transform") continue;
			if (ImGui::Button((std::string("Remove##") + typeName).c_str())) {
				FlEntityComponentSystemKernel::Instance().RemoveComponent(typeName, id);
				FlEditorAdministrator::Instance().GetLogger()->AddLog("Removed component %s from %u", typeName.c_str(), id);
				compTypes = FlEntityComponentSystemKernel::Instance().GetEntityComponentTypes(id);
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Add Component");

	// すべての登録コンポーネント名を取得
	const auto& allTypes = FlEntityComponentSystemKernel::Instance().GetRegisteredComponentTypes();

	// 既にアタッチ済みのものは除外
	std::vector<const char*> attachableTypes;
	attachableTypes.reserve(allTypes.size());

	for (const auto& typeName : allTypes)
	{
		bool alreadyAttached = false;
		for (const auto& existingComp : FlEntityComponentSystemKernel::Instance().GetEntityComponentTypes(id))
		{
			if (existingComp == typeName)
			{
				alreadyAttached = true;
				break;
			}
		}
		if (!alreadyAttached)
			attachableTypes.push_back(typeName.c_str());
	}

	// attachableTypes が空なら何も追加できない
	if (attachableTypes.empty())
		ImGui::Text("No components available to attach.");
	else
	{
		static int selectedIndex = 0;
		// インデックスが範囲外にならないよう調整
		if (selectedIndex >= attachableTypes.size())
			selectedIndex = 0;

		const char* preview = attachableTypes[selectedIndex];

		if (ImGui::BeginCombo("##ComponentTypes", preview))
		{
			for (int i = 0; i < attachableTypes.size(); ++i) {
				bool selected = (selectedIndex == i);

				if (ImGui::Selectable(attachableTypes[i], selected))
					selectedIndex = i;

				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();

		// ▼ Attach ボタン
		if (ImGui::Button("Attach"))
		{
			try {
				FlEntityComponentSystemKernel::Instance().AddComponent(attachableTypes[selectedIndex], id);
			}
			catch (...) {

			}
		}
	}

    ImGui::End();
}

void FlECSInspectorAndHierarchy::RenderEntityNode(uint32_t id)
{
    auto& kernel = FlEntityComponentSystemKernel::Instance();

    // 名前取得
    std::string name = "Entity";
    if (auto nc = static_cast<NameComponent*>(kernel.GetComponent("Name", id)))
        name = nc->m_name;

    // 変換コンポーネント
    auto* tc = static_cast<TransformComponent*>(kernel.GetComponent("Transform", id));

    bool selected = (m_selectedEntityId == id);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        (selected ? ImGuiTreeNodeFlags_Selected : 0);

    if (tc && tc->m_children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx((void*)(uintptr_t)id, flags, "%s (ID %u)", name.c_str(), id);

    bool isActive = ImGui::IsItemActive();
    bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);

    // 左クリックで選択
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        m_selectedEntityId = id;

    // ---- Drag Source ----
    if (isActive && isDragging)
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("ENTITY_ID", &id, sizeof(uint32_t));
            ImGui::Text("Move %s", name.c_str());
            ImGui::EndDragDropSource();
        }
    }

    // ---- Drop Target ----
    if (ImGui::BeginDragDropTarget())
    {
        if (auto payload = ImGui::AcceptDragDropPayload("ENTITY_ID"))
        {
            uint32_t childId = *(uint32_t*)payload->Data;
            if (childId != id)
                SetParent(childId, id);
        }
        ImGui::EndDragDropTarget();
    }

    // ---- Right Click Context Menu ----
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
        {
            DeleteEntityRecursive(id);
            ImGui::EndPopup();
            if (open) ImGui::TreePop();
            return;
        }
		if (ImGui::MenuItem("Termination of Parent"))
			SetParent(id, UINT32_MAX);
		if (ImGui::MenuItem("Create Prefab"))
			CreatePrefab(id);
		if (ImGui::MenuItem("Instantiate Prefab"))
		{
			std::string path{ "Assets/Prefabs/" };
			if (OpenFileDialog(path, "Load Prefab", "Prefab (*.flprefab)\0*.flprefab\0"))
				InstantiatePrefab(path);
		}

        ImGui::EndPopup();
    }

    if (open)
    {
        if (tc)
        {
            for (auto child : tc->m_children)
                RenderEntityNode(child);
        }
        ImGui::TreePop();
    }
}

void FlECSInspectorAndHierarchy::DeleteEntityRecursive(uint32_t id)
{
    auto& kernel = FlEntityComponentSystemKernel::Instance();
    auto* tc = static_cast<TransformComponent*>(kernel.GetComponent("Transform", id));

    if (tc)
    {
        // 子から順番に削除
        for (uint32_t child : tc->m_children)
            DeleteEntityRecursive(child);

        // 親から自身を取り除く
        if (tc->m_parent != UINT32_MAX)
        {
            auto* parentTC = static_cast<TransformComponent*>(
                kernel.GetComponent("Transform", tc->m_parent));

            if (parentTC)
            {
                parentTC->m_children.erase(
                    std::remove(parentTC->m_children.begin(), parentTC->m_children.end(), id),
                    parentTC->m_children.end()
                );
            }
        }
    }

    // 実際のエンティティ削除
    kernel.DestroyEntity(id);

    // 選択中だったらクリア
    if (m_selectedEntityId == id)
        m_selectedEntityId = UINT32_MAX;

    // Hierarchy 再描画用
    RefreshEntityList();

    FlEditorAdministrator::Instance().GetLogger()->AddLog(
        "Deleted Entity %u (and children)", id
    );
}

void FlECSInspectorAndHierarchy::SetParent(uint32_t childId, uint32_t newParentId)
{
    auto& kernel = FlEntityComponentSystemKernel::Instance();

    auto* childTC = static_cast<TransformComponent*>(kernel.GetComponent("Transform", childId));

    auto* parentTC = (newParentId != UINT32_MAX) ?
        static_cast<TransformComponent*>(kernel.GetComponent("Transform", newParentId)) : nullptr;

    if (!childTC || (newParentId != UINT32_MAX && !parentTC)) return;

    // 自己親子付けを防止
    if (childId == newParentId) return;

    // --- 現在の親から削除 ---
    if (childTC->m_parent != UINT32_MAX)
    {
        auto* oldParentTC = static_cast<TransformComponent*>(kernel.GetComponent("Transform", childTC->m_parent));
        if (oldParentTC)
        {
            auto& children = oldParentTC->m_children;
            children.erase(
                std::remove(children.begin(), children.end(), childId),
                children.end()
            );
        }
    }

    if (parentTC)
        parentTC->m_children.push_back(childId);

    // 親IDを更新 (UINT32_MAX または新しい親ID)
    childTC->m_parent = newParentId;

    // Log
    FlEditorAdministrator::Instance().GetLogger()->AddLog(
        "SetParent: %u to %u", childId, newParentId
    );

    // 親子関係の変更により、ルートエンティティ一覧が変わるため更新
    RefreshEntityList();
}

void FlECSInspectorAndHierarchy::CreatePrefab(uint32_t root)
{
	std::string filepath{ "Assets/Prefabs/" };
	if (!SaveFileDialog(filepath, "Save Prefab",
		"Prefab Files (*.flprefab)\0*.flprefab\0", "flprefab"))
		return;

	auto& kernel = FlEntityComponentSystemKernel::Instance();

	std::vector<uint32_t> entities;
	CollectEntitiesRecursive(kernel, root, entities);

	nlohmann::json prefab;
	prefab["Root"] = root;

	for (auto id : entities)
		prefab["Entities"][std::to_string(id)] = kernel.SerializeEntity(id);

	FlJsonUtility::Serialize(prefab, filepath);
}

void FlECSInspectorAndHierarchy::InstantiatePrefab(const std::string& path)
{
	nlohmann::json prefab;
	if (!FlJsonUtility::Deserialize(prefab, path)) return;

	std::unordered_map<uint32_t, uint32_t> remap;
	FlEntityComponentSystemKernel::Instance()
		.DeserializeScene(prefab, &remap);

	uint32_t newRoot = remap[prefab["Root"].get<uint32_t>()];

	// ルート化
	if (auto tc = static_cast<TransformComponent*>(
		FlEntityComponentSystemKernel::Instance().GetComponent("Transform", newRoot)))
	{
		tc->m_parent = UINT32_MAX;
	}

	RefreshEntityList();
}

bool FlECSInspectorAndHierarchy::OpenFileDialog(std::string& filepath, const std::string& title, const char* filters)
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

bool FlECSInspectorAndHierarchy::SaveFileDialog(std::string& filepath, const std::string& title, const char* filters, const std::string& defExt)
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
