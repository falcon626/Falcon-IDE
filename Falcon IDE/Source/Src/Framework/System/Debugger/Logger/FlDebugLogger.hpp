#pragma once

/// <summary>
/// デバッグログをファイルに書き込むためのクラス。
/// </summary>
class DebugLogger
{
public:
    /// <summary>
    /// コンストラクタ。指定されたファイルにログを書き込みます。
    /// </summary>
    /// <param name="filename">ログファイルのパス。デフォルトは "Assets/Data/Log/DebugLog.log"。</param>
    explicit DebugLogger(const std::string_view& filename = "Assets/Data/Log/DebugLog.log")
    {
        m_logFile.open(filename, std::ios::out | std::ios::app);
        m_upFPM = std::make_unique<FlFilePathManager>();
        if (!m_logFile) _ASSERT_EXPR(false, L"ログファイルがありません");
    }

    /// <summary>
    /// デストラクタ。ログファイルを閉じます。
    /// </summary>
    ~DebugLogger() { if (m_logFile.is_open()) m_logFile.close(); }

    /// <summary>
    /// デバッグメッセージをログファイルに書き込みます。
    /// </summary>
    /// <param name="message">ログに書き込むメッセージ。</param>
    /// <param name="file">メッセージが発生したファイル名。</param>
    /// <param name="line">メッセージが発生した行番号。</param>
    void LogDebug(const std::string& message, const char* file, int line)
    {
        if (m_logFile.is_open())
        {
            auto refl{ m_upFPM->GetRelative(file).generic_string() };

            m_logFile << GetCurrentDateTime() << std::endl
                << "[File: " << refl << "] "
                << "[Line: " << line << "] "
                << message << std::endl;
        }
    }

    /// <summary>
    /// ログファイルが開いているかどうかを判定します。
    /// </summary>
    /// <returns>ログファイルが開いていれば true、そうでなければ false を返します。</returns>
    const auto IsOpen() const noexcept { return m_logFile.is_open(); }

private:
    /// <summary>
    /// ログファイルのストリーム。
    /// </summary>
    std::ofstream m_logFile;

    std::unique_ptr<FlFilePathManager> m_upFPM;

    /// <summary>
    /// 現在の日時を取得します。
    /// </summary>
    /// <returns>現在の日時を表す文字列。</returns>
    std::string GetCurrentDateTime()
    {
        auto now        { std::chrono::high_resolution_clock::now() };
        auto now_time_t { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
        auto duration   { now.time_since_epoch() };

        auto seconds    { std::chrono::duration_cast<std::chrono::seconds>(duration) };
        auto nanoseconds{ std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds) };

        auto localTime  { std::tm{} };
#ifdef _WIN32
        localtime_s(&localTime, &now_time_t);
#else  // !_WIN32
        localtime_r(&now_time_t, &localTime);
#endif
        auto oss        { std::ostringstream{} };
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "."
            << std::setfill('0') << std::setw(9) << nanoseconds.count();
        return oss.str();
    }
};

/// <summary>
/// デバッグログを記録するマクロ。
/// </summary>
/// <param name="logger">DebugLoggerのインスタンス。</param>
/// <param name="message">ログに書き込むメッセージ。</param>
#define DEBUG_LOG(logger, message)logger->LogDebug(message, __FILE__, __LINE__)

