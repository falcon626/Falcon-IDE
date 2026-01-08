#pragma once

class FlInspectorAndHierarchyEditor
{
public:
	void RenderInspector(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
    void RenderHierarchy(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

private:
    void RenderEntityRecursive(const std::weak_ptr<Entity>& entity);

    bool OpenFileDialog(std::string& filepath, const std::string& title, const char* filters);
    bool SaveFileDialog(std::string& filepath, const std::string& title, const char* filters, const std::string& defExt);

    std::weak_ptr<Entity> m_selectedEntity;
};
