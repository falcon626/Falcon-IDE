export module FlProcessCreater;

/**
 * @brief 外部コマンドを実行し、その標準出力と標準エラーをキャプチャして返します。
 * * @param cmdLine 実行するコマンドライン文字列 (例: L"dotnet sln ...")
 * @param outStdOutput [out] キャプチャされた標準出力/標準エラー (UTF-8またはOEM CP)
 * @param outExitCode [out] プロセスの終了コード
 * @return bool 関数の実行が成功したかどうか (プロセス起動の成否)
 */
export bool ExecuteCommand(const std::wstring& cmdLine,
        std::string& outStdOutput,
        DWORD& outExitCode)
{
    outStdOutput.clear();
    outExitCode = static_cast<DWORD>(-Def::IntOne); // 初期値

    HANDLE hPipeRead  = NULL;
    HANDLE hPipeWrite = NULL;

    // 1. パイプの作成 (標準出力/エラーをリダイレクトするため)
    SECURITY_ATTRIBUTES saAttr = { 0 };
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE; // 子プロセスに書き込みハンドルを継承させる
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0)) {
        std::wcerr << L"CreatePipe failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    // 読み取りハンドルが継承されないように設定 (重要)
    if (!SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0)) {
        std::wcerr << L"SetHandleInformation failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return false;
    }

    // 2. プロセス起動情報 (STARTUPINFO) の設定
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // ウィンドウを非表示
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE); // 標準入力はそのまま
    si.hStdOutput = hPipeWrite; // 標準出力をパイプ書き込み口に
    si.hStdError = hPipeWrite;  // 標準エラーもパイプ書き込み口に

    PROCESS_INFORMATION pi = { 0 };

    // CreateProcessW はコマンドライン引数を変更する可能性があるため、
    // const な wstring から可変バッファにコピーする
    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0'); // ヌル終端

    // 3. プロセスの作成
    BOOL bSuccess = CreateProcessW(
        NULL,           // アプリケーション名
        cmdBuffer.data(), // コマンドライン
        NULL,           // プロセスセキュリティ属性
        NULL,           // スレッドセキュリティ属性
        TRUE,           // ハンドル継承 (bInheritHandles = TRUE)
        CREATE_NO_WINDOW, // ウィンドウを作成しない
        NULL,           // 環境変数
        NULL,           // カレントディレクトリ
        &si,            // STARTUPINFO
        &pi             // PROCESS_INFORMATION
    );

    if (!bSuccess) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("CreateProcessW failed. Error: %lu", GetLastError());
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return false;
    }

    // 4. 親プロセス側ではパイプの「書き込み」ハンドルをすぐに閉じる
    // これをしないと、子プロセス終了後も ReadFile がブロックし続ける
    CloseHandle(hPipeWrite);
    hPipeWrite = NULL;

    // 5. パイプから出力データを読み取る
    const DWORD BUFFER_SIZE = Def::BitMaskPos13;
    // unique_ptr を使って自動的にメモリを解放する
    auto readBuffer = std::make_unique<char[]>(BUFFER_SIZE);

    DWORD dwRead = 0;
    while (ReadFile(hPipeRead, readBuffer.get(), BUFFER_SIZE, &dwRead, NULL) && dwRead > 0)
    {
        // 読み取ったバイトデータをそのまま string に追加
        // (文字コード変換は行わない。dotnet の場合は UTF-8 が期待される)
        outStdOutput.append(readBuffer.get(), dwRead);
    }

    // 6. プロセスの終了を待機
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 7. 終了コードを取得
    GetExitCodeProcess(pi.hProcess, &outExitCode);

    // 8. 残りのハンドルをクリーンアップ
    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
};