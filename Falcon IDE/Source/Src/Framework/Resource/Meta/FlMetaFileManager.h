#pragma once

class FlMetaFileManager
{
public:

	FlMetaFileManager() = default;
	~FlMetaFileManager() { StopMonitoring(); }

	/// <summary>
	/// 指定ディレクトリ以下を監視し、.FlMetaフォルダにメタファイルを生成
	/// </summary>
	/// <param name="rootPath">監視対象のルートディレクトリ
	/// <param name="interval">監視間隔（秒）
	void StartMonitoring(const std::string& rootPath, int intervalSeconds = Def::IntOne);

	/// <summary>
	/// 監視を停止
	/// </summary>
	void StopMonitoring() { m_fileWatcher.Stop(); }

	/// <summary>
	/// アセットパスに対応する.metaファイルが存在しない場合、新規に作成
	/// </summary>
	/// <param name="assetPath">アセットのファイルパス
	void CreateMetaFileIfNotExist(const std::string& assetPath);


	/// <summary>
	/// アセットが名前変更または移動された場合呼び出す関数
	/// </summary>
	/// <param name="oldPath">以前のファイルパス</param>
	/// <param name="newPath">以降のファイルパス</param>
	void OnAssetRenamedOrMoved(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);

	/// <summary>
	/// 指定アセットのloadFlagをtrueに設定
	/// </summary>
	/// <param name="assetPath">アセットのファイルパス
	void IncrementLoadFlag(const std::string& assetPath);

	/// <summary>
	/// GUIDでアセットのパスを検索
	/// </summary>
	/// <param name="guid">検索するGUID
	/// <return>マッチしたパス（見つからない場合空文字列）</return>
	const std::optional<std::string> FindAssetByGuid(const std::string& guid) const;

	/// <summary>
	/// アセットパスでGUIDを検索
	/// </summary>
	/// <param name="path">検索するアセットパス
	/// <return>GUID（見つからない場合空文字列）</return>
	const std::optional<std::string> FindGuidByAsset(const std::filesystem::path& path) const;

	/// <summary>
	/// 監視対象のファイルの全パス所得（ディレクトリ、メタファイルを除く）
	/// </summary>
	/// <returns>監視対象の全ファイルパス</returns>
	const std::list<std::string> GetAllFilePaths() const;

	/// <summary>
	/// 指定したアセットが変更されたかの判定
	/// </summary>
	/// <param name="assetPath">アセットのパス</param>
	/// <returns>変更されたかどうか（変更されていた場合true）</returns>
	const bool IsAssetChanged(const std::filesystem::path& assetPath) const;

	/// <summary>
	/// アセットの変更フラグをリセット
	/// </summary>
	/// <param name="assetPath">アセットのパス</param>
	void ResetAssetChangeFlag(const std::filesystem::path& assetPath);

	/// <summary>
	/// GUIDマップの参照を所得
	/// </summary>
	/// <returns>GUIDマップの参照</returns>
	const auto& GetGuidMap() const noexcept { return m_guidMap; }

private:
	/// <summary>
	/// .FlMetaフォルダ内のパスかどうかを判定
	/// </summary>
	bool IsInsideFlMeta(const std::filesystem::path& path) const;

	/// <summary>
	/// .FlMetaフォルダのパスを取得
	/// </summary>
	std::filesystem::path GetMetaFolderPath(const std::filesystem::path& assetPath) const;

	/// <summary>
	/// メタファイルからGUIDを抽出
	/// </summary>
	const std::optional<std::string> GetGuidFromMetaFile(const std::string& metaPath) const;

	/// <summary>
	/// ファイルまたはディレクトリのメタファイルを作成または更新
	/// </summary>
	void CreateOrUpdateFlMetaFile(const std::filesystem::path& assetPath);

	/// <summary>
	/// 秘匿メタフォルダがあるかどうか判断しなければ作成する
	/// </summary>
	/// <param name="path">メタフォルダのパス</param>
	void ExistsMetaFolder(const std::filesystem::path& path) const noexcept;

	/// <summary>
	/// アセットが変更されたかをチェック
	/// </summary>
	bool IsAssetModified(const std::filesystem::path& assetPath, const nlohmann::json& metaJson) const;

	/// <summary>
	/// ファイル監視時のコールバック
	/// </summary>
	void OnFileEvent(const std::filesystem::path& path, FlFileWatcher::FileStatus status);

	FlFileWatcher m_fileWatcher;
	std::string m_metaFileExtension = ".flmeta";
	std::filesystem::path m_rootPath;

	// <K:GUID V:Path>
	std::unordered_map<std::string, std::string> m_guidMap;
};