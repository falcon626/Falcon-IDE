#pragma once

class FlFileWatcher 
{
public:
    enum class FileStatus : uint32_t
    {
        Created,
        Modified,
        Erased
    };

	FlFileWatcher() = default;
    ~FlFileWatcher() { Stop(); }

    using Callback = std::function<void(const std::filesystem::path&, FileStatus)>;

    void SetPathAndInterval(const std::filesystem::path& pathToWatch, std::chrono::duration<int> interval);
    void SetPath(const std::filesystem::path& pathToWatch) noexcept;
	void SetInterval(std::chrono::duration<int> interval) noexcept { m_interval = interval; }

    void Start(Callback callback);
    void Stop();

    bool AddFile(const std::filesystem::path& path, const std::string& content);
    bool RemFile(const std::filesystem::path& path);
	bool RenameFile(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);

	bool AddDirectory(const std::filesystem::path& path);
	bool RemDirectory(const std::filesystem::path& path);
    bool RenameDirectory(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);

    bool Move(const std::filesystem::path& src, const std::filesystem::path& dst);

    bool Copy(const std::filesystem::path& src, const std::filesystem::path& dst);

private:
    bool Contains(const std::string& key);

    std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;
    std::filesystem::path m_pathToWatch;
    std::chrono::duration<int> m_interval{ std::chrono::duration<int>(Def::IntZero) };
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
};

