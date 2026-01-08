#pragma once

class Console final
{
private:
    HANDLE m_hConsole{ INVALID_HANDLE_VALUE };
    mutable std::mutex m_mutex;

    // コンソールハンドルの初期化
    void InitConsoleHandle()
    {
        m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_hConsole == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to get console handle");
        }
    }

public:
    // インスタンス取得（スレッドセーフなシングルトン）
    static Console& Instance()
    {
        static Console instance;
        return instance;
    }

    // コピー/ムーブ禁止
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    // コンソールクリア
    void Clear() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(m_hConsole, &csbi)) return;

        COORD topLeft = { 0, 0 };
        DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
        DWORD written;

        FillConsoleOutputCharacterW(m_hConsole, L' ', cellCount, topLeft, &written);
        FillConsoleOutputAttribute(m_hConsole, csbi.wAttributes, cellCount, topLeft, &written);
        SetConsoleCursorPosition(m_hConsole, topLeft);
    }

    // ライン描画
    void LINE(int num = 1)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const std::wstring line = L"------------------------------------------------------------------------\n";
        DWORD written;

        for (int i = 0; i < num; ++i) {
            WriteConsoleW(m_hConsole, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
        }
    }

    // 書き込み操作
    template<typename T>
    void Write(const T& out)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::wostringstream wss;
        wss << time_acquisition() << L" | " << out << L'\n';

        std::wstring message = wss.str();
        DWORD written;
        WriteConsoleW(m_hConsole, message.c_str(), static_cast<DWORD>(message.size()), &written, nullptr);
    }

    // 演算子オーバーロード
    template<typename T>
    Console& operator<<(const T& out)
    {
        Write(out);
        return *this;
    }

private:
    Console()
    {
        if (!AllocConsole()) {
            throw std::runtime_error("Failed to allocate console");
        }

        // コンソール設定
        SetConsoleTitleW(L"デバッグ用コンソール");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
            FOREGROUND_INTENSITY | FOREGROUND_GREEN);

        // 閉じるボタン無効化
        HMENU hMenu = GetSystemMenu(GetConsoleWindow(), FALSE);
        RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

        // ロケール設定
        setlocale(LC_ALL, "Japanese");
        InitConsoleHandle();
    }

    ~Console()
    {
        if (m_hConsole != INVALID_HANDLE_VALUE) {
            FreeConsole();
        }
    }

    // 時刻取得（スレッドセーフ）
    std::wstring time_acquisition() const
    {
        std::time_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );
        std::wstring timeStr(20, L'\0');

        tm localTime;
        localtime_s(&localTime, &now);
        std::wcsftime(const_cast<wchar_t*>(timeStr.c_str()), timeStr.size(), L"%Y-%m-%d %H:%M:%S", &localTime);

        return timeStr;
    }
};