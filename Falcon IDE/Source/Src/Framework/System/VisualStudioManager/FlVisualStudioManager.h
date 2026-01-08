#pragma once

#include "../System/XMLParser/FlAutomaticFileAddSystem.h"
#include "../System/CppParser/FlCppParser.h"

class FlVisualStudioProjectManager
{
public:
    FlVisualStudioProjectManager(const std::filesystem::path& solutionPath)
        : m_solutionPath{ solutionPath }
        , m_autoAdd{ std::make_unique<FlAutomaticFileAddSystem>() }
        , m_cppParser{ std::make_unique<FlSimpleCppParser>() }
    {}

    // プロジェクト作成とソリューションへの追加を一括実行
    bool CreateNewProject(const std::string& projectName,
        const std::filesystem::path& targetDir) noexcept;

    bool RemoveProjectFromSolution(const std::filesystem::path& projPath) noexcept;

    bool FormingModule(const std::filesystem::path& projDir, const std::filesystem::path& codeFile) noexcept;

private:

    // -----------------------------------------------
    // 初期コードファイル生成
    // -----------------------------------------------
    bool CreateSourceFiles(const std::filesystem::path& dir,
        const std::string& name) noexcept;

    // -----------------------------------------------
    // プロジェクトファイル生成（.vcxproj）
    // -----------------------------------------------
    bool CreateVcxproj(const std::filesystem::path& dir,
        const std::string& name) noexcept;

    // -----------------------------------------------
    // フィルターファイル作成 (.filters)
    // -----------------------------------------------
    bool CreateFilters(const std::filesystem::path& dir,
        const std::string& name) noexcept;

    // -----------------------------------------------
    // dotnet CLI を使い .sln にプロジェクトを追加
    // -----------------------------------------------
    bool AddToSolutionUsingDotNet(const std::filesystem::path& sln,
        const std::filesystem::path& vcxproj) noexcept;

    // -----------------------------------------------
    // .sln に直接プロジェクト追加
    // -----------------------------------------------
    bool AddProjectToSolution(
        const std::filesystem::path& sln,
        const std::filesystem::path& dir,
        const std::string& name
        ) noexcept;

    // -----------------------------------------------
    // テキスト置換（ファイル単位）
    // -----------------------------------------------
    void ReplaceInFile(const std::filesystem::path& path,
        const std::string& from,
        const std::string& to) noexcept;

    // -----------------------------------------------
    // GUID置換
    // -----------------------------------------------
    void ReplaceInFile(const std::filesystem::path& path,
        const std::string& from) noexcept;

    std::filesystem::path m_solutionPath;

    std::unique_ptr<FlAutomaticFileAddSystem> m_autoAdd;
    std::unique_ptr<FlSimpleCppParser> m_cppParser;
};
