#pragma once

class FlECSInspectorAndHierarchy
{
public:
    FlECSInspectorAndHierarchy();
    ~FlECSInspectorAndHierarchy() = default;

    // 描画メイン
    void Render(const char* hierarchyTitle, const char* inspectorTitle, bool* p_openHierarchy, bool* p_openInspector);

    void RefreshEntityList(); // kernel から取り直す
private:
    // Hierarchy 側
    void RenderHierarchyWindow(const char* title, bool* p_open);
    //void RenderEntityNode(entityId id);

    // Inspector 側
    void RenderInspectorWindow(const char* title, bool* p_open);
    void RenderEntityNode(uint32_t id);
    void SetParent(uint32_t childId, uint32_t newParentId);

    void CreatePrefab(uint32_t root);

    void InstantiatePrefab(const std::string& path);

    void DeleteEntityRecursive(uint32_t id);

    std::vector<uint32_t> GetRootEntities();

    bool OpenFileDialog(std::string& filepath, const std::string& title, const char* filters);
    bool SaveFileDialog(std::string& filepath, const std::string& title, const char* filters, const std::string& defExt);

    // 選択
    uint32_t m_selectedEntityId{ UINT32_MAX };

    // キャッシュ（一覧、更新は RefreshEntityList）
    std::vector<uint32_t> m_entityList;

    // Add component UI
    int m_selectedRegisteredTypeIndex = 0;
};
