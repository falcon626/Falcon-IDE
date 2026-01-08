#pragma once

class FlFileEditor
{
public:

	FlFileEditor() noexcept;

    /// <summary>
    /// アセットブラウザウィンドウを表示します。
    /// </summary>
    /// <param name="title">ウィンドウのタイトル文字列。</param>
    /// <param name="p_open">ウィンドウの開閉状態を制御するためのブール値へのポインタ。NULLの場合は制御しません。</param>
    /// <param name="flags">ウィンドウの表示オプションを指定するフラグ。デフォルトは ImGuiWindowFlags_None です。</param>
    void ShowAssetBrowser(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None) noexcept;

    /// <summary>
    /// ドロップされたファイルのパスを設定します。
    /// </summary>
    /// <param name="path">設定するファイルのパス。</param>
    auto SetDroppedFile(const std::filesystem::path& path) noexcept { m_doppedPath = path; }
private:
    struct FileEntry
    {
        std::filesystem::path path;
        bool isDirectory = false;
        bool isOpen      = false; // ImGuiのツリー状態
        std::vector<FileEntry> children;
    };

    struct FileEntryUIState 
    {
        std::string newName;
        std::string renameName;
        bool showCreatePopup = false;
        bool showRenamePopup = false;
        bool createAsDirectory        = true;
        bool isRenamePopupNewlyOpened = true;
    };

    struct PopupRequest 
    {
        std::filesystem::path target;
        enum class Type { None, Rename, Create, Delete } type = Type::None;
    };

    void BuildFileTree(const std::filesystem::path& root, FileEntry& entry) noexcept;
    void RenderFileEntry(FileEntry& entry) noexcept;
	void RenderPopup() noexcept;
    void RenderFileItem(const std::filesystem::directory_entry& entry) noexcept;

    void RenderCurrentItem();

	FlFileWatcher m_fileWatcher; // ファイル監視用
    std::unordered_map<std::filesystem::path, FileEntryUIState> m_uiStates;
	std::optional<PopupRequest> m_pendingPopup; // ポップアップ要求

    std::filesystem::path m_doppedPath{};
    std::filesystem::path m_oldEntryPath{};
    std::filesystem::path m_basePath{ "Assets" };
    std::filesystem::path m_currentPath = m_basePath;

    std::unique_ptr<FlFilePathManager> m_upFPM;
};