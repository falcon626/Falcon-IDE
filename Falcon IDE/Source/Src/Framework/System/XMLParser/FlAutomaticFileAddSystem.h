#pragma once
#include "tinyxml2.h"

/// <summary>
/// 作成するファイルの構成情報
/// </summary>
struct FileCreationConfig
{
    std::string fileName;                    // 例: "MyClass.cpp", "Header.ixx"
    std::string content;                     // ファイルに書き込む内容 (テンプレート置換済み)
    std::string itemType{ Def::EmptyStr };   // vcxproj上の属性 (例: "ClCompile", "ClInclude", "None")
};

class FlAutomaticFileAddSystem
{
public:
    FlAutomaticFileAddSystem(const std::filesystem::path& vcxprojPath,
        const std::filesystem::path& filtersPath);

    FlAutomaticFileAddSystem();

    /// <summary>
    /// 任意の数のファイルを作成し、プロジェクトに追加する
    /// </summary>
    /// <param name="files">作成するファイルのリスト</param>
    /// <param name="filterPath">VS上のフィルタ階層 (例: "Src\Logic\Player")</param>
    /// <returns>成功したらtrue</returns>
    bool AddFiles(const std::vector<FileCreationConfig>& files, const std::string& filterPath) noexcept;

    /// <summary>
    /// 指定されたファイルをプロジェクトとディスクから削除する
    /// </summary>
    bool RemoveFiles(const std::vector<std::string>& fileNames, const std::string& filterPath) noexcept;

    /// <summary>
    /// Projectディレクトリパスのセッタ（Projectファイルやフィルタファイルパスの自動設定）
    /// </summary>
    /// <param name="projectDir">Projectディレクトリのパス</param>
    void SetProjectDirPath(std::filesystem::path projectDir) noexcept;

private:
    // ファイルシステム操作
    bool WriteFileToDisk(const std::filesystem::path& fullPath, const std::string& content) noexcept;
    bool DeleteFileFromDisk(const std::filesystem::path& fullPath) noexcept;

    // XML操作 (ドキュメントを引数で受け取る形に変更し、連続処理に対応)
    bool AddToVcxproj(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath, const std::string& itemType) noexcept;
    bool AddToFilters(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath, const std::string& itemType, const std::string& filterPathStr) noexcept;

    bool RemoveFromVcxproj(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath) noexcept;
    bool RemoveFromFilters(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath) noexcept;

    // フィルタ階層作成ヘルパー
    void EnsureFilterPathExists(tinyxml2::XMLDocument& doc, const std::string& filterPathStr) noexcept;

    std::string DetectItemType(const std::filesystem::path& filePath) noexcept;

    std::filesystem::path m_vcxprojPath;
    std::filesystem::path m_filtersPath;
    std::filesystem::path m_projectDir;
    const std::unordered_map<std::string, std::string> TypeMap;
};