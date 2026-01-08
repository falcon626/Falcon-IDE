#pragma once

class FlPythonMacroEditor
{
public:
    // コンストラクタで出力先のターミナルエディタを受け取る
    FlPythonMacroEditor(FlTerminalEditor& terminal);

    // ImGuiウィンドウを描画する
    void RenderEditor(const std::string& title, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);

private:
    // 指定されたパスのPythonスクリプトを実行する
    void ExecuteScript(const std::filesystem::path& scriptPath);

    // "Assets/Macro"ディレクトリをスキャンしてスクリプト一覧を更新する
    void RefreshScriptList();

    // 出力先のターミナルエディタへの参照
    FlTerminalEditor& m_terminal;

    // "Assets/Macro"ディレクトリのパス
    const std::filesystem::path m_macroDirectory = "Assets/Macro";

    // 見つかったスクリプトファイルのパス一覧
    std::vector<std::filesystem::path> m_scriptPaths;
};