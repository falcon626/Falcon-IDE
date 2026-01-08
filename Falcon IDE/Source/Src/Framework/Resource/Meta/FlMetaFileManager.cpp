#include "FlMetaFileManager.h"

bool FlMetaFileManager::IsInsideFlMeta(const std::filesystem::path& path) const
{
	auto current{ path };
	while (!current.empty() && current != current.parent_path()) {
		if (current.filename() == ".FlMeta") return true;
		current = current.parent_path();
	}
	return false;
}

void FlMetaFileManager::StartMonitoring(const std::string& rootPath, int intervalSeconds)
{
	m_rootPath = std::filesystem::path(rootPath);
	std::filesystem::create_directories(m_rootPath);

	// 初期スキャン: 既存のファイル/ディレクトリにメタファイルを作成または更新
	for (const auto& entry : std::filesystem::recursive_directory_iterator(m_rootPath)) {
		if (!IsInsideFlMeta(entry.path())) CreateOrUpdateFlMetaFile(entry.path());
	}

	// ルートディレクトリのメタファイルを作成または更新
	CreateOrUpdateFlMetaFile(m_rootPath);

	// ファイル監視開始
	auto absoluteRootPath{ std::filesystem::absolute(rootPath) };
	m_fileWatcher.SetPathAndInterval(absoluteRootPath.string() , std::chrono::seconds(intervalSeconds));
	m_fileWatcher.Start([this](const std::filesystem::path& path, FlFileWatcher::FileStatus status) {
		OnFileEvent(path, status);
		});
}

void FlMetaFileManager::CreateMetaFileIfNotExist(const std::string& assetPath)
{
	if (!std::filesystem::exists(assetPath)) return;
	auto metaPath{ GetMetaFolderPath(assetPath) / (std::filesystem::path(assetPath).filename().string() + m_metaFileExtension) };
	if (std::filesystem::exists(metaPath)) return;
	CreateOrUpdateFlMetaFile(assetPath);
}

void FlMetaFileManager::OnAssetRenamedOrMoved(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
{
	if (IsInsideFlMeta(oldPath) || IsInsideFlMeta(newPath)) return;

	// 元メタファイルパス
	auto oldMetaFolder{ GetMetaFolderPath(oldPath) };
	auto oldMetaFile{ oldMetaFolder / (oldPath.filename().string() + m_metaFileExtension) };

	// 新メタファイルパス
	auto newMetaFolder{ GetMetaFolderPath(newPath) };
	auto newMetaFile{ newMetaFolder / (newPath.filename().string() + m_metaFileExtension) };

	// 古いメタファイルが存在しなければ無視
	if (!std::filesystem::exists(oldMetaFile)) return;

	// 新しい .FlMeta フォルダがなければ作成
	ExistsMetaFolder(newMetaFolder);

	// メタファイル移動
	if (!m_fileWatcher.RenameFile(oldMetaFile, newMetaFile)) return;

	// 内部GUIDマップ更新
	auto guid{ GetGuidFromMetaFile(newMetaFile.string()) };
	if (guid.has_value())
		m_guidMap[guid.value()] = newPath.string();

	// アセットパス情報を更新
	auto metaJson{ nlohmann::json{} };
	if (FlJsonUtility::Deserialize(metaJson, newMetaFile))
	{
		metaJson["assetPath"] = newPath.string();
		if(!FlJsonUtility::Serialize(metaJson, newMetaFile))
			FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to Serialize assetPath %s", newMetaFile.string().c_str());
	}
	else FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to Deserialize assetPath %s", newMetaFile.string().c_str());
}

void FlMetaFileManager::IncrementLoadFlag(const std::string& assetPath)
{
	if (!std::filesystem::exists(assetPath)) return;
	auto metaFolder{ GetMetaFolderPath(assetPath) };
	auto metaFileName{ std::filesystem::path(assetPath).filename().string() + m_metaFileExtension };
	auto metaPath{ metaFolder / metaFileName };
	// メタファイルが存在しない場合、作成
	if (!std::filesystem::exists(metaPath)) CreateOrUpdateFlMetaFile(assetPath);
	// メタファイルを読み込み
	auto metaJson{ nlohmann::json{} };
	if (!FlJsonUtility::Deserialize(metaJson, metaPath)) return;

	auto loadFlag{ false };
	FlJsonUtility::GetValue(metaJson, "loadFlag", &loadFlag);
	if (loadFlag) return;
	metaJson["loadFlag"] = true;
	// 更新を保存
	if (!FlJsonUtility::Serialize(metaJson, metaPath)) 
		FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed Serialize loadFlag %s", metaPath.string().c_str());
}

const std::optional<std::string> FlMetaFileManager::FindAssetByGuid(const std::string& guid) const
{
	auto it{ m_guidMap.find(guid) };
	if (it != m_guidMap.end()) return it->second;
	else return std::nullopt;
}

const std::optional<std::string> FlMetaFileManager::FindGuidByAsset(const std::filesystem::path& path) const
{
	auto metaFolder{ GetMetaFolderPath(path) };
	auto metaFileName{ path.filename().string() + m_metaFileExtension };
	auto metaPath{ metaFolder / metaFileName };

	if (!std::filesystem::exists(metaPath)) return std::nullopt;

	auto guid{ GetGuidFromMetaFile(metaPath.string()) };
	return guid;
}

const std::list<std::string> FlMetaFileManager::GetAllFilePaths() const
{
	std::list<std::string> filePaths;

	// ルートが存在しない場合は空のリストを返す
	if (!std::filesystem::exists(m_rootPath)) return filePaths;

	// 再帰的にスキャン
	for (const auto& entry : std::filesystem::recursive_directory_iterator(m_rootPath))
	{
		// 1. .FlMeta 隠しフォルダの中身は無視する
		if (IsInsideFlMeta(entry.path())) continue;

		// 2. ディレクトリ自体はリストに含めない
		if (std::filesystem::is_directory(entry.path())) continue;

		// 3. 管理用メタファイル (*.FlMeta) 自体はアセットではないので無視する
		if (entry.path().extension() == m_metaFileExtension) continue;

		// 上記の除外条件を抜けた「純粋なアセットファイル」のみ追加
		filePaths.push_back(entry.path().string());
	}

	return filePaths;
}

const bool FlMetaFileManager::IsAssetChanged(const std::filesystem::path& assetPath) const
{
	// ファイルが存在しない、またはメタファイルがない場合は変更なしとみなす（あるいはエラー扱い）
	if (!std::filesystem::exists(assetPath)) return false;

	auto metaFolder{ GetMetaFolderPath(assetPath) };
	auto metaFileName{ assetPath.filename().string() + m_metaFileExtension };
	auto metaPath{ metaFolder / metaFileName };

	if (!std::filesystem::exists(metaPath)) return false;

	// メタファイルを読み込み
	auto metaJson{ nlohmann::json{} };
	if (!FlJsonUtility::Deserialize(metaJson, metaPath)) return false;

	auto isChanged{ false };
	FlJsonUtility::GetValue(metaJson, "isChanged", &isChanged);

	return isChanged;
}

void FlMetaFileManager::ResetAssetChangeFlag(const std::filesystem::path& assetPath)
{
	if (!std::filesystem::exists(assetPath)) return;

	auto metaFolder{ GetMetaFolderPath(assetPath) };
	auto metaFileName{ assetPath.filename().string() + m_metaFileExtension };
	auto metaPath{ metaFolder / metaFileName };

	if (!std::filesystem::exists(metaPath)) return;

	// メタファイルを読み込み
	auto metaJson{ nlohmann::json{} };
	if (FlJsonUtility::Deserialize(metaJson, metaPath))
	{
		// 既に false なら書き込み処理をスキップして負荷を減らす
		auto currentStatus{ false };
		FlJsonUtility::GetValue(metaJson, "isChanged", &currentStatus);

		if (currentStatus) 
		{
			metaJson["isChanged"] = false;
			FlJsonUtility::Serialize(metaJson, metaPath);
		}
	}
	else FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to Deserialize isChanged %s", metaPath.string().c_str());
}

std::filesystem::path FlMetaFileManager::GetMetaFolderPath(const std::filesystem::path& assetPath) const
{
	if (assetPath == m_rootPath) return assetPath / ".FlMeta";
	return assetPath.parent_path() / ".FlMeta";
}

const std::optional<std::string> FlMetaFileManager::GetGuidFromMetaFile(const std::string& metaPath) const
{
	auto metaJson{ nlohmann::json{} };
	auto metaExists{ std::filesystem::exists(metaPath) };
	auto existingGuid{ std::string{} };
	if (metaExists)
	{
		if (FlJsonUtility::Deserialize(metaJson, metaPath)) 
			FlJsonUtility::GetValue(metaJson, "Guid", &existingGuid);
		else FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to Deserialize Guid %s", metaPath.c_str());
	}

	if (!existingGuid.empty()) return existingGuid;
	else return std::nullopt;
}

void FlMetaFileManager::ExistsMetaFolder(const std::filesystem::path& path) const noexcept
{
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directories(path);
#ifdef _WIN32
		SetFileAttributesW(path.wstring().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
	}
}

bool FlMetaFileManager::IsAssetModified(const std::filesystem::path& assetPath, const nlohmann::json& metaJson) const
{
	std::string recordedTime;
	if (std::filesystem::is_regular_file(assetPath)) {
		FlJsonUtility::GetValue(metaJson, "lastModified", &recordedTime);
	}
	else if (std::filesystem::is_directory(assetPath)) {
		FlJsonUtility::GetValue(metaJson, "lastUpdated", &recordedTime);
	}
	else {
		return true; // 不明な場合更新
	}
	auto ftime = std::filesystem::last_write_time(assetPath);
	auto sysTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
	);
	auto currentTimeStr = std::format("{:%Y-%m-%dT%H:%M:%SZ}", sysTime);
	return recordedTime != currentTimeStr;
}

void FlMetaFileManager::CreateOrUpdateFlMetaFile(const std::filesystem::path& assetPath)
{
	if (assetPath.filename() == ".FlMeta") return;
	// .FlMetaフォルダ作成
	auto metaFolder{ GetMetaFolderPath(assetPath) };
	ExistsMetaFolder(metaFolder);
	// メタファイルパス生成
	auto metaFileName{ assetPath.filename().string() + m_metaFileExtension };
	auto metaPath{ metaFolder / metaFileName };
	// 既存のメタファイルを読み込む
	auto metaJson{ nlohmann::json{} };
	auto existingGuid{ std::string{} };
	auto metaExists{ std::filesystem::exists(metaPath) };
	if (metaExists)
	{
		if (FlJsonUtility::Deserialize(metaJson, metaPath)) {
			FlJsonUtility::GetValue(metaJson, "Guid", &existingGuid);
			// 変更がない場合スキップ
			if (!IsAssetModified(assetPath, metaJson)) {
				m_guidMap[existingGuid] = assetPath.string();
				return;
			}
		}
		else FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to Deserialize Guid %s", metaPath.string().c_str());
	}
	// GUIDを設定（既存のものを保持、なければ新規生成）
	if (existingGuid.empty())
	{
		auto guid{ FlGuid{} };
		guid.NewGuid();

		auto strGuid{ guid.ToString() };
		metaJson["Guid"] = strGuid;

		m_guidMap[strGuid] = assetPath.string();
	}
	else
	{
		metaJson["Guid"] = existingGuid;
		m_guidMap[existingGuid] = assetPath.string();
	}

	// 基本情報を設定
	metaJson["assetPath"] = assetPath.u8string();
	metaJson["isDirectory"] = std::filesystem::is_directory(assetPath);

	// ファイルの場合、最終更新日時を記録
	if (std::filesystem::is_regular_file(assetPath))
	{
		auto ftime{ std::filesystem::last_write_time(assetPath) };
		auto sysTime{ std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
		) };
		auto timeStr{ std::format("{:%Y-%m-%dT%H:%M:%SZ}", sysTime) };
		metaJson["lastModified"] = timeStr;
	}

	// ディレクトリの場合、更新時刻と子リストを記録
	else if (std::filesystem::is_directory(assetPath))
	{
		auto ftime{ std::filesystem::last_write_time(assetPath) };
		auto sysTime{ std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
		) };
		auto timeStr{ std::format("{:%Y-%m-%dT%H:%M:%SZ}", sysTime) };

		metaJson["lastUpdated"] = timeStr;

		auto children{ std::vector<std::string>{} };
		for (const auto& entry : std::filesystem::directory_iterator(assetPath)) {
			if (entry.path().filename() != ".FlMeta") children.push_back(entry.path().filename().string());
		}
		metaJson["children"] = children;
	}
	metaJson["loadFlag"] = false;
	metaJson["isChanged"] = true;

	// メタファイル書き込み
	if(FlJsonUtility::Serialize(metaJson, metaPath))FlEditorAdministrator::Instance().GetLogger()->AddChangeLogU8(u8"Create/Update Meta: %s", metaPath.u8string().c_str());
	else FlEditorAdministrator::Instance().GetLogger()->AddErrorLogU8(u8"Failed to Create/Update Meta: %s", metaPath.u8string().c_str());
}

void FlMetaFileManager::OnFileEvent(const std::filesystem::path& path, FlFileWatcher::FileStatus status)
{
	if (IsInsideFlMeta(path) || path.extension() == m_metaFileExtension) return;
	switch (status)
	{
	case FlFileWatcher::FileStatus::Created:
		CreateOrUpdateFlMetaFile(path);
		{
			auto parentPath{ path.parent_path() };
			if (!IsInsideFlMeta(parentPath)) CreateOrUpdateFlMetaFile(parentPath);
		}
		break;
	case FlFileWatcher::FileStatus::Modified:
		if (std::filesystem::is_regular_file(path)) CreateOrUpdateFlMetaFile(path);
		break;
	case FlFileWatcher::FileStatus::Erased:
		{
			auto metaPath{ GetMetaFolderPath(path) / (path.filename().string() + m_metaFileExtension) };

			if(GetGuidFromMetaFile(metaPath.string()).has_value())
				m_guidMap[GetGuidFromMetaFile(metaPath.string()).value()].clear();
			else
			{
				auto isPathValid{ [](const std::string& p) {
					if (p.empty()) return false;

					std::error_code ec;
					auto status = std::filesystem::status(p, ec);

					if (ec) return false;
					if (!std::filesystem::exists(status)) return false;

					return true;
					} 
				};

				std::erase_if(m_guidMap, [isPathValid](const auto& pair) {
					return !isPathValid(pair.second);
				});
			}

			if (std::filesystem::exists(metaPath)) m_fileWatcher.RemFile(metaPath);
		}
		{
			auto parentPath{ path.parent_path() };
			if (!IsInsideFlMeta(parentPath) && std::filesystem::exists(parentPath)) CreateOrUpdateFlMetaFile(parentPath);
		}
		break;
	}
}