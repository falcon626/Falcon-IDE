#include "FlFileWatcher.h"

void FlFileWatcher::SetPathAndInterval(const std::filesystem::path& pathToWatch, std::chrono::duration<int> interval)
{
    if (pathToWatch.is_absolute())
        m_pathToWatch = pathToWatch;
    else
        m_pathToWatch = std::filesystem::absolute(pathToWatch);

    m_interval    = interval;

    // 初期化: 監視対象のパスに存在するファイルのタイムスタンプを取得
    for (const auto& file : std::filesystem::recursive_directory_iterator(m_pathToWatch)) {
        const auto pathStr{ file.path().string() };
        m_paths[pathStr]   = std::filesystem::last_write_time(file);
	}
}

void FlFileWatcher::SetPath(const std::filesystem::path& pathToWatch) noexcept
{
    if (pathToWatch.is_absolute())
        m_pathToWatch = pathToWatch;
    else
        m_pathToWatch = std::filesystem::absolute(pathToWatch);

    for (const auto& file : std::filesystem::recursive_directory_iterator(m_pathToWatch)) {
        const auto pathStr{ file.path().string() };
        m_paths[pathStr] = std::filesystem::last_write_time(file);
    }
}

bool FlFileWatcher::Contains(const std::string& key) 
{
    return m_paths.find(key) != m_paths.end();
}

void FlFileWatcher::Start(Callback callback) 
{
    m_running = true;

    m_thread = std::thread([this, callback]() {
        while (m_running) {
            std::this_thread::sleep_for(m_interval);

            try {
                for (auto& file : std::filesystem::recursive_directory_iterator(m_pathToWatch)) {
                    const auto pathStr = file.path().string();
                    const auto lastWriteTime = std::filesystem::last_write_time(file);

                    if (!Contains(pathStr)) 
                    {
                        m_paths[pathStr] = lastWriteTime;
                        callback(file.path(), FileStatus::Created);
                    }
                    else 
                    {
                        if (m_paths[pathStr] != lastWriteTime) 
                        {
                            m_paths[pathStr] = lastWriteTime;
                            callback(file.path(), FileStatus::Modified);
                        }
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Filesystem error: %s  Path: %s", e.what(), m_pathToWatch.c_str());
            }

            // 削除されたファイルの検出
            auto it{ m_paths.begin() };
            while (it != m_paths.end()) {
                if (!std::filesystem::exists(it->first)) 
                {
                    callback(it->first, FileStatus::Erased);
                    it = m_paths.erase(it);
                }
                else ++it;
            }
        }
    });
}

bool FlFileWatcher::AddFile(const std::filesystem::path& path, const std::string& content)
{
    try {
        auto ofs{ std::ofstream{path} };
        ofs << content;
        ofs.close();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::RemFile(const std::filesystem::path& path) 
{
    try {
        return std::filesystem::remove(path);
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::RenameFile(const std::filesystem::path& oldPath, const std::filesystem::path& newPath) 
{
    try {
        std::filesystem::rename(oldPath, newPath);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::AddDirectory(const std::filesystem::path& path)
{
    try {
        return std::filesystem::create_directories(path);
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::RemDirectory(const std::filesystem::path& path)
{
    try {
        return std::filesystem::remove_all(path) > NULL;
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::RenameDirectory(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
{
    try {
        std::filesystem::rename(oldPath, newPath);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::Move(const std::filesystem::path& src, const std::filesystem::path& dst)
{
    try {
        std::filesystem::rename(src, dst);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FlFileWatcher::Copy(const std::filesystem::path& src, const std::filesystem::path& dst)
{
    try {
        if (std::filesystem::is_directory(src)) std::filesystem::copy(src, dst, 
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

        else std::filesystem::copy_file(src, dst, 
            std::filesystem::copy_options::overwrite_existing);

        return true;
    }
    catch (...) {
        return false;
    }
}

void FlFileWatcher::Stop() 
{
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}
