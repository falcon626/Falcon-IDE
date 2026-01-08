#include "FlSolutionParser.h"

bool FlSolutionParser::Load(const std::filesystem::path& slnPath)
{
    m_projects.clear();
    m_solutionPath = slnPath;

    std::ifstream file(slnPath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("Project(", 0) == 0) 
        {
            // すべてのダブルクォートの位置を集める
            std::vector<size_t> quotes;
            for (size_t pos = 0; pos < line.size(); ++pos) {
                if (line[pos] == '"') quotes.push_back(pos);
            }

            if (quotes.size() >= 6)
            {
                // "プロジェクト名"
                std::string name = line.substr(quotes[2] + 1, quotes[3] - quotes[2] - 1);
                // "プロジェクトパス"
                std::string relPath = line.substr(quotes[4] + 1, quotes[5] - quotes[4] - 1);

                // 前後の空白を削除
                auto trim{ [](std::string& s) {
                    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                        return !std::isspace(ch);
                        }));
                    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                        return !std::isspace(ch);
                        }).base(), s.end());
                    } };
                trim(relPath);

                // バックスラッシュをスラッシュに変換
                std::replace(relPath.begin(), relPath.end(), '\\', '/');

                std::filesystem::path projectPath = m_solutionPath.parent_path() / relPath;

                std::string ext = projectPath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".vcxproj") 
                {
                    FlProjectInfo info;
                    info.name = name;
                    info.relativePath = relPath;
                    info.fullPath = std::filesystem::absolute(projectPath).lexically_normal();
                    m_projects.push_back(info);
                }
            }
        }
    }
    return !m_projects.empty();
}


const FlProjectInfo* FlSolutionParser::FindProjectByName(const std::string& name) const
{
    for (const auto& proj : m_projects) {
        if (proj.name == name) 
            return &proj;
    }
    return nullptr;
}
