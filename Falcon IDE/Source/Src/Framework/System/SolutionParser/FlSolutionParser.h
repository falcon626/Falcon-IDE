#pragma once

// ソリューションに含まれるプロジェクト情報
struct FlProjectInfo 
{
    std::string name;                  // プロジェクト名
    std::filesystem::path relativePath;// .vcxproj への相対パス
    std::filesystem::path fullPath;    // 絶対パス
};

class FlSolutionParser
{
public:
    FlSolutionParser() = default;
    explicit FlSolutionParser(const std::filesystem::path& slnPath) {
        Load(slnPath);
    }

    // .sln ファイルを読み込んでプロジェクトを解析
    bool Load(const std::filesystem::path& slnPath);

    // プロジェクトリストを取得
    const std::vector<FlProjectInfo>& GetProjects() const { return m_projects; }

    // 特定のプロジェクトを名前で検索
    const FlProjectInfo* FindProjectByName(const std::string& name) const;
private:
    std::filesystem::path m_solutionPath;
    std::vector<FlProjectInfo> m_projects;
};
