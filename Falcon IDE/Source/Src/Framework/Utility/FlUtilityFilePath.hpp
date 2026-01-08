#pragma once

class FlFilePathManager
{
public:
    FlFilePathManager() 
        : m_directoryHierarchyLevels{ std::to_underlying(Directorys::ThisFile) } 
        , m_baseFilePath{ GetBaseFilePath() }
    {}

    /// <summary>
    /// #Getter 指定されたパスを基準パスからの相対パスに変換します。
    /// </summary>
    /// <param name="target">相対パスを取得したい対象のパス。</param>
    /// <returns>基準パス（m_baseFilePath）から見た target の相対パス。</returns>
    const auto GetRelative(const std::filesystem::path& target) const noexcept
    { 
        if (target.is_absolute()) return std::filesystem::relative(target, m_baseFilePath);

        // ベースディレクトリ配下のすべてのファイルを探索
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_baseFilePath)) {
            if (!entry.is_regular_file()) continue;

            auto relPath{ std::filesystem::relative(entry.path(), m_baseFilePath) };
            if (relPath.string().ends_with(target.string())) return relPath;
        }

        return std::filesystem::path{};
    }

    /// <summary>
    /// #Getter 指定されたパスを相対パスから絶対パスに変換します。
    /// </summary>
    /// <param name="target">絶対パスを取得したい対象のパス。</param>
    /// <returns>targetの絶対パス　例外で空のパス</returns>
    const std::filesystem::path GetAbsolute(const std::filesystem::path& target) const
    {
        try {
            // 絶対パスに変換
            auto absolutePath{ std::filesystem::absolute(target) };
            return absolutePath;
        }
        catch (...) {
			return std::filesystem::path{}; // エラー時は空のパスを返す
        }
    }

    /// <summary>
    /// #Setter ディレクトリ階層レベルを設定します。
    /// </summary>
    /// <param name="directoryHierarchyLevels">設定するディレクトリ階層レベル。省略時は Directorys::ThisFile の値が使用されます。</param>
    auto SetHierarchyLevels(const uint32_t directoryHierarchyLevels 
        = std::to_underlying(Directorys::ThisFile)) noexcept
    {
        m_directoryHierarchyLevels = directoryHierarchyLevels;
        m_baseFilePath             = GetBaseFilePath();
    }

private:
    /// <summary>
    /// #Getter ファイルのパスから指定された階層分だけ親ディレクトリをたどり、基底ファイルパスを取得します。
    /// </summary>
    /// <returns>指定されたディレクトリ階層分だけ親ディレクトリをたどった後の、基底ファイルパス（std::filesystem::path型）。</returns>
    const std::filesystem::path GetBaseFilePath() const noexcept
    {
        auto currentFile{ std::filesystem::path(__FILE__).lexically_normal() };

        for (auto i{ Def::UIntZero }; i < m_directoryHierarchyLevels; ++i) {
            currentFile = currentFile.parent_path();
            if (currentFile.empty()) [[unlikely]]
            {
                assert(false && "Current File Is Empty");
                return std::filesystem::path{};
            }
        }
        return currentFile;
    }

    enum class Directorys : uint32_t
    {
        ProjectDir,
         Src,
          Framework,
           Utility,
            ThisFile
    };
   
    uint32_t m_directoryHierarchyLevels;
    std::filesystem::path m_baseFilePath;
};