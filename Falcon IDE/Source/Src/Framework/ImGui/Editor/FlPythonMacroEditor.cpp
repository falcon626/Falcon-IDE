#include "FlPythonMacroEditor.h"

FlPythonMacroEditor::FlPythonMacroEditor(FlTerminalEditor& terminal)
    : m_terminal(terminal)
{
    RefreshScriptList();
}

void FlPythonMacroEditor::RenderEditor(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    if (ImGui::Begin(title.c_str(), p_open, flags))
    {
        if (ImGui::Button("Refresh Macros"))
        {
            RefreshScriptList();
        }
        ImGui::Separator();

        if (m_scriptPaths.empty())
        {
            ImGui::Text("No scripts found in %s", m_macroDirectory.string().c_str());
        }
        else
        {
            for (const auto& path : m_scriptPaths)
            {
                // ボタンのラベルとしてファイル名を使用
                std::string buttonLabel = path.filename().string();
                if (ImGui::Button(buttonLabel.c_str()))
                {
                    ExecuteScript(path);
                }
            }
        }
    }
    ImGui::End();
}

void FlPythonMacroEditor::RefreshScriptList()
{
    m_scriptPaths.clear();
    m_terminal.AddLog("> Scanning for scripts in " + m_macroDirectory.string());

    // ディレクトリが存在するかチェック
    if (!std::filesystem::exists(m_macroDirectory))
    {
        m_terminal.AddLog("> Error: Macro directory not found: " + m_macroDirectory.string());
        return;
    }

    // ディレクトリ内のファイルをイテレート
    for (const auto& entry : std::filesystem::directory_iterator(m_macroDirectory))
    {
        // .py拡張子を持つ通常のファイルのみを対象とする
        if (entry.is_regular_file() && entry.path().extension() == ".py")
        {
            m_scriptPaths.push_back(entry.path());
        }
    }

    m_terminal.AddLog("> Found " + std::to_string(m_scriptPaths.size()) + " scripts.");
}


void FlPythonMacroEditor::ExecuteScript(const std::filesystem::path& scriptPath)
{
    if (!std::filesystem::exists(scriptPath))
    {
        m_terminal.AddLog("> Error: Script file not found: " + scriptPath.string());
        return;
    }

    std::string command = "py.exe \"" + std::filesystem::absolute(scriptPath).string() + "\"";

    m_terminal.ExecuteCommand(command.c_str());
}