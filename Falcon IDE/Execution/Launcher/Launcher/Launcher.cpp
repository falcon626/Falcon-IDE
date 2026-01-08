#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace fs = std::filesystem;

// -------------------- 文字列ユーティリティ --------------------
static inline std::string Trim(std::string s) {
	auto notSpace = [](unsigned char c) { return !std::isspace(c); };
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
	s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
	return s;
}

static inline std::string ToLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return (char)std::tolower(c); });
	return s;
}

static std::wstring Utf8ToWide(const std::string& utf8) {
	if (utf8.empty()) return L"";
	int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		utf8.data(), (int)utf8.size(),
		nullptr, 0);
	if (len <= 0) return L"";

	std::wstring w(len, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		utf8.data(), (int)utf8.size(),
		w.data(), len);
	return w;
}

static std::string ReadAllBytes(const fs::path& p) {
	std::ifstream ifs(p, std::ios::binary);
	if (!ifs) return {};
	std::ostringstream oss;
	oss << ifs.rdbuf();
	return oss.str();
}

// UTF-8 BOM 除去
static void StripUtf8Bom(std::string& s) {
	if (s.size() >= 3 &&
		(unsigned char)s[0] == 0xEF &&
		(unsigned char)s[1] == 0xBB &&
		(unsigned char)s[2] == 0xBF) {
		s.erase(0, 3);
	}
}

// -------------------- cfg パース --------------------
static std::unordered_map<std::string, std::string> ParseCfgUtf8(const fs::path& cfgPath) {
	std::unordered_map<std::string, std::string> kv;

	std::string bytes = ReadAllBytes(cfgPath);
	if (bytes.empty()) return kv;

	StripUtf8Bom(bytes);

	std::istringstream iss(bytes);
	std::string line;
	while (std::getline(iss, line)) {
		// CR除去
		if (!line.empty() && line.back() == '\r') line.pop_back();

		line = Trim(line);
		if (line.empty()) continue;
		if (line[0] == '#' || line[0] == ';') continue;

		auto eq = line.find('=');
		if (eq == std::string::npos) continue;

		std::string key = ToLower(Trim(line.substr(0, eq)));
		std::string val = Trim(line.substr(eq + 1));

		kv[key] = val;
	}
	return kv;
}

// -------------------- ランチャーの場所取得 --------------------
static fs::path GetThisExeDir() {
	wchar_t buf[MAX_PATH]{};
	DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	fs::path exePath = (n > 0) ? fs::path(buf) : fs::current_path();
	return exePath.parent_path();
}

static void ShowErrorBox(const std::wstring& msg, const std::wstring& title = L"Launcher") {
	MessageBoxW(nullptr, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
}

// -------------------- メイン --------------------
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	fs::path selfDir = GetThisExeDir();
	fs::path cfgPath = selfDir / L"launcher.cfg";

	if (!fs::exists(cfgPath)) {
		ShowErrorBox(L"launcher.cfg が見つかりません。\n\n場所:\n" + cfgPath.wstring());
		return 1;
	}

	auto kv = ParseCfgUtf8(cfgPath);

	auto get = [&](const std::string& key, const std::string& def = "") -> std::string {
		auto it = kv.find(key);
		return (it != kv.end()) ? it->second : def;
		};

	std::string targetUtf8 = get("target");
	if (targetUtf8.empty()) {
		ShowErrorBox(L"launcher.cfg に target= が設定されていません。");
		return 2;
	}

	std::string argsUtf8 = get("args", "");
	std::string workdirUtf8 = get("workdir", "");
	std::string showErrUtf8 = ToLower(get("show_error", "true"));

	bool showError = (showErrUtf8 != "false" && showErrUtf8 != "0" && showErrUtf8 != "no");

	// target のパス解決（相対なら Launcher.exe の場所から）
	fs::path targetPath = fs::path(Utf8ToWide(targetUtf8));
	if (targetPath.is_relative()) {
		targetPath = (selfDir / targetPath);
	}

	// 正規化（存在確認前に弱い正規化：.. の整理）
	std::error_code ec;
	targetPath = fs::weakly_canonical(targetPath, ec);
	if (ec) {
		// canonical化に失敗してもそのまま使える場合があるので、ここでは致命にしない
		targetPath = fs::path(Utf8ToWide(targetUtf8));
		if (targetPath.is_relative()) targetPath = (selfDir / targetPath);
	}

	if (!fs::exists(targetPath)) {
		if (showError) {
			ShowErrorBox(L"起動対象の exe が見つかりません。\n\ntarget:\n" + targetPath.wstring());
		}
		return 3;
	}

	// workdir（空なら target のフォルダ）
	fs::path workdirPath;
	if (!workdirUtf8.empty()) {
		workdirPath = fs::path(Utf8ToWide(workdirUtf8));
		if (workdirPath.is_relative()) workdirPath = (selfDir / workdirPath);
		workdirPath = fs::weakly_canonical(workdirPath, ec);
		if (ec) {
			// 失敗時はそのまま
			workdirPath = fs::path(Utf8ToWide(workdirUtf8));
			if (workdirPath.is_relative()) workdirPath = (selfDir / workdirPath);
		}
	}
	else {
		workdirPath = targetPath.parent_path();
	}

	std::wstring targetW = targetPath.wstring();
	std::wstring argsW = Utf8ToWide(argsUtf8);
	std::wstring workW = workdirPath.wstring();

	// ShellExecuteExW で起動（Unicode対応）
	SHELLEXECUTEINFOW sei{};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = L"open";
	sei.lpFile = targetW.c_str();
	sei.lpParameters = argsW.empty() ? nullptr : argsW.c_str();
	sei.lpDirectory = workW.empty() ? nullptr : workW.c_str();
	sei.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteExW(&sei)) {
		DWORD err = GetLastError();
		if (showError) {
			std::wstring msg =
				L"起動に失敗しました。\n\nexe:\n" + targetW +
				L"\n\nエラーコード: " + std::to_wstring(err);
			ShowErrorBox(msg);
		}
		return 4;
	}

	// ランチャーは即終了（必要なら Wait するオプションも追加できます）
	if (sei.hProcess) CloseHandle(sei.hProcess);
	return 0;
}