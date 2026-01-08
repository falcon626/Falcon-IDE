#pragma once

namespace Str // String Series
{
	/// <summary>
	/// ファイルパスから拡張子を抽出します。
	/// </summary>
	/// <param name="filePath">拡張子を取得する対象のファイルパス。</param>
	/// <returns>ファイルパスに拡張子が含まれていればその拡張子（ドットを除く文字列）、含まれていなければ空文字列を返します。</returns>
	[[nodiscard(L"Extension Not Used")]] inline const auto FileExtensionSearcher(const std::string& filePath) noexcept
	{
		const auto dotPos{ filePath.rfind('.') };
		if (dotPos != std::string::npos) return filePath.substr(dotPos + Def::UIntOne);

		return std::string{ "" };
	}

	/// <summary>
	/// 文字列包含判定関数
	/// </summary>
	/// <param name="haystack">検索対象となる文字列。</param>
	/// <param name="needle">検索する部分文字列。</param>
	/// <returns>部分文字列が含まれていれば true、含まれていなければ false を返します。</returns>
	[[nodiscard(L"Result Not Used")]] inline const auto Contains(const std::string& haystack, const std::string& needle) 
	{
		// find関数で部分文字列を検索
		return haystack.find(needle) != std::string::npos;
	}

	/// <summary>
	/// 指定した文字を別の文字に置き換えた新しい文字列を返します。
	/// </summary>
	/// <param name="str">置換対象となる元の文字列。</param>
	/// <param name="oldChar">置換したい文字。</param>
	/// <param name="newChar">新しく置き換える文字。</param>
	/// <returns>oldChar を newChar に置き換えた新しい std::string。</returns>
	[[nodiscard(L"Result Not Used")]] inline const auto ReplaceChar(const std::string& str, char oldChar, char newChar) noexcept
	{
		auto result{ str };

		for (auto& c : result) 
			if (c == oldChar) c = newChar;

		return result;
	}


	/// <summary>
	/// 指定した文字列内のすべての部分文字列を別の文字列に置換します。
	/// </summary>
	/// <param name="str">置換対象となる元の文字列。</param>
	/// <param name="oldStr">置換したい部分文字列。</param>
	/// <param name="newStr">置換後の文字列。</param>
	/// <returns>すべての一致する部分文字列が置換された新しい文字列。</returns>
	[[nodiscard(L"Result Not Used")]] inline const auto ReplaceString(const std::string& str, const std::string& oldStr, const std::string& newStr)
	{
		auto result{ str };
		auto pos   { size_t{} };

		while ((pos = result.find(oldStr, pos)) != std::string::npos) 
		{
			result.replace(pos, oldStr.length(), newStr);
			pos += newStr.length();
		}

		return result;
	}

	/// <summary>
	/// 指定したファイル内のすべての部分文字列を別の文字列に変換します。
	/// </summary>
	/// <param name="filePath">置換対象となるファイルのパス。</param>
	/// <param name="oldString">置換したい文字列。</param>
	/// <param name="newString">置換後の文字列。</param>
	/// <returns>成功した場合true</returns>
	inline const auto ReplaceStringInFile(const std::filesystem::path& filePath, const std::string& oldString, const std::string& newString) noexcept
	{
		if (!std::filesystem::exists(filePath)) return false;

		// ファイル読み込み
		std::ifstream inFile(filePath, std::ios::in | std::ios::binary);
		if (!inFile) return false;

		std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
		inFile.close();

		// 置換（すべての出現箇所を置換）
		size_t pos = 0;
		while ((pos = content.find(oldString, pos)) != std::string::npos) {
			content.replace(pos, oldString.length(), newString);
			pos += newString.length();
		}

		// ファイル書き戻し
		std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
		if (!outFile) return false;

		outFile << content;
		outFile.close();

		return true;
	}

	/// <summary>
	/// Windows用改行コード(CRLF)に変換します。
	/// </summary>
	/// <param name="s">変換対象文字列</param>
	/// <returns>変換後文字列</returns>
	[[nodiscard(L"Result Not Used")]] inline const auto ConvertToCRLF(const std::string& s) 
	{
		auto out{ std::string{} };
		out.reserve(s.size() * 2);

		for (auto i{ Def::ULongLongZero }; i < s.size(); ++i)
		{
			auto c{ s[i] };
			if (c == '\r')
			{
				out += "\r\n";
				if (i + 1 < s.size() && s[i + 1] == '\n') ++i;
			}
			else if (c == '\n') out += "\r\n";
			else out += c;
		}

		return out;
	}

	/// <summary>
	/// 文字列内に指定された文字の出現回数をカウントします。
	/// </summary>
	/// <param name="s">文字列</param>
	/// <param name="c">カウント対象文字</param>
	/// <returns>カウント回数</returns>
	[[nodiscard(L"Result Not Used")]] inline const auto CountChar(const std::string& s, char c)
	{
		auto cnt{ Def::IntZero };
		for (char ch : s) if (ch == c) cnt++;
		return cnt;
	}

	/// <summary>
	/// 指定されたディレクトリ内で、指定された拡張子を持つファイルのパス一覧を取得します。
	/// </summary>
	/// <param name="directory">検索対象となるディレクトリのパス。</param>
	/// <param name="exte">検索するファイルの拡張子（例: ".txt"）。</param>
	/// <returns>条件に一致するファイルパスのリスト</returns>
	[[nodiscard(L"File Paths Not Used")]] inline const auto GetFilePaths(const std::filesystem::path& directory, const std::string_view& exte)
	{
		auto filePaths{ std::vector<std::string>{} };

		for (const auto& entry : std::filesystem::directory_iterator(directory))
			if (entry.path().extension() == exte) filePaths.emplace_back(entry.path().lexically_normal().string());

		return filePaths;
	}

	/// <summary>
	/// 文字列をハッシュ値（数値）に変換します。
	/// </summary>
	/// <param name="str">ハッシュ化する対象の文字列。</param>
	/// <returns>入力された文字列のハッシュ値（数値型）。</returns>
	[[nodiscard(L"Convert Number Not Used")]] inline const auto StringToNumber(const std::string& str) noexcept
	{
		auto hash_fn{ std::hash<std::string>{} };
		return hash_fn(str);
	}

	/// <summary>
	/// その文字列がUTF8"ぽい"か判断する
	/// </summary>
	/// <param name="text">判定したい文字列</param>
	/// <returns>UTF8ぽかったらtrue</returns>
	[[nodiscard(L"Convert Number Not Used")]] inline const auto IsLikelyUtf8(const std::string& text)
	{
		auto bytes{ reinterpret_cast<const unsigned char*>(text.data()) };
		auto len { text.size() };
		auto i{ Def::ULongLongZero };

		while (i < len)
		{
			if (bytes[i] <= 0x7F) {
				++i; // ASCII
			}
			else if ((bytes[i] & 0xE0) == 0xC0) {
				if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80) return false;
				i += 2;
			}
			else if ((bytes[i] & 0xF0) == 0xE0) {
				if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;
				i += 3;
			}
			else if ((bytes[i] & 0xF8) == 0xF0) {
				if (i + 3 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
				i += 4;
			}
			else {
				return false;
			}
		}
		return true;
	}

	/// <summary>
	/// 2つの文字列を数値として比較し、等しいかどうかを判定します。
	/// </summary>
	/// <param name="str1">比較する最初の文字列。</param>
	/// <param name="str2">比較する2番目の文字列。</param>
	/// <returns>2つの文字列を数値に変換した結果が等しい場合はtrue、そうでない場合はfalseを返します。</returns>
	[[nodiscard(L"String True False Not Used")]] inline const auto CompareStringsAsNumbers(const std::string& str1, const std::string& str2) noexcept
	{
		return StringToNumber(str1) == StringToNumber(str2);
	}

	/// <summary>
	/// 指定されたフォーマット文字列と可変引数を使用して、フォーマットされた文字列を作成します。
	/// </summary>
	/// <param name="fmt">フォーマット文字列</param>
	/// <param name="...">フォーマットに対応する可変引数</param>
	/// <returns>フォーマット中にエラーが発生した場合、空の文字列を返します。</returns>
	[[nodiscard(L"FromatString Not Used")]] inline const auto FormatString(const char* fmt, ...)
	{
		auto args{ va_list{} };
		va_start(args, fmt);
		auto size{ vsnprintf(nullptr, NULL, fmt, args) };
		va_end(args);

		if (size < NULL) { return std::string{ "" }; }

		auto result{ std::string(size, '\0') };
		va_start(args, fmt);
		vsnprintf(&result[Def::UIntZero], static_cast<size_t>(size) + Def::ULongLongOne, fmt, args);
		va_end(args);

		return result;
	}

	template <typename... Args>
	[[nodiscard(L"FormatStringF Not Used")]]
	inline std::string FormatStringF(std::string_view fmt, Args&&... args)
	{
		return std::format(fmt, std::forward<Args>(args)...);
	}

	// ワイド文字版（C言語スタイル）
	/// <summary>
	/// 指定されたワイドフォーマット文字列と可変引数を使用して、フォーマットされたワイド文字列を作成します。
	/// </summary>
	/// <param name="fmt">ワイドフォーマット文字列</param>
	/// <param name="...">フォーマットに対応する可変引数</param>
	/// <returns>フォーマット中にエラーが発生した場合、空のワイド文字列を返します。</returns>
	[[nodiscard(L"FormatStringW Not Used")]] inline const auto FormatStringW(const wchar_t* fmt, ...)
	{
		auto args{ va_list{} };
		va_start(args, fmt);
		auto size{ vswprintf(nullptr, NULL, fmt, args) };
		va_end(args);

		if (size < NULL) { return std::wstring{ L"" }; }

		auto result{ std::wstring(size, L'\0') };
		va_start(args, fmt);
		vswprintf(&result[Def::UIntZero], static_cast<size_t>(size) + Def::ULongLongOne, fmt, args);
		va_end(args);

		return result;
	}

	// ワイド文字版
	template <typename... Args>
	[[nodiscard(L"FormatStringFW Not Used")]]
	inline std::wstring FormatStringFW(std::wstring_view fmt, Args&&... args)
	{
		return std::format(fmt, std::forward<Args>(args)...);
	}

	// UTF-8文字列版（std::format使用）
	/// <summary>
	/// std::formatを使用してUTF-8文字列をフォーマットします。
	/// </summary>
	/// <param name="fmt">フォーマット文字列</param>
	/// <param name="args">フォーマット引数</param>
	/// <returns>フォーマットされたUTF-8文字列</returns>
	template <typename... Args>
	[[nodiscard(L"FormatStringFU8 Not Used")]]
	inline std::u8string FormatStringFU8(std::u8string_view fmt, Args&&... args)
	{
		// std::formatは直接u8stringを返さないため、stringで処理してから変換
		auto temp = std::format(reinterpret_cast<const char*>(fmt.data()), std::forward<Args>(args)...);
		return std::u8string(reinterpret_cast<const char8_t*>(temp.c_str()), temp.size());
	}

	// UTF-8文字列版（C言語スタイル - 必要に応じて）
	/// <summary>
	/// 指定されたUTF-8フォーマット文字列と可変引数を使用して、フォーマットされたUTF-8文字列を作成します。
	/// </summary>
	/// <param name="fmt">UTF-8フォーマット文字列</param>
	/// <param name="...">フォーマットに対応する可変引数</param>
	/// <returns>フォーマット中にエラーが発生した場合、空のUTF-8文字列を返します。</returns>
	[[nodiscard(L"FormatStringU8 Not Used")]] inline const auto FormatStringU8(const char8_t* fmt, ...)
	{
		auto args{ va_list{} };
		va_start(args, fmt);
		// char8_tをcharとして扱ってvsnprintfを使用
		auto size{ vsnprintf(nullptr, NULL, reinterpret_cast<const char*>(fmt), args) };
		va_end(args);

		if (size < NULL) { return std::u8string{ u8"" }; }

		auto temp{ std::string(size, '\0') };
		va_start(args, fmt);
		vsnprintf(&temp[Def::UIntZero], static_cast<size_t>(size) + Def::ULongLongOne,
			reinterpret_cast<const char*>(fmt), args);
		va_end(args);

		// stringからu8stringに変換
		return std::u8string(reinterpret_cast<const char8_t*>(temp.c_str()), temp.size());
	}

	/// <summary>
	/// std::u8stringをstd::stringに安全に変換します（範囲ベース）
	/// </summary>
	/// <param name="u8str">変換元のUTF-8文字列</param>
	/// <returns>変換されたstd::string</returns>
	[[nodiscard(L"U8StringToStringSafe Not Used")]] inline std::string U8StringToStringSafe(const std::u8string& u8str)
	{
		return std::string(u8str.begin(), u8str.end());
	}

	/// <summary>
	/// std::u8string_viewをstd::stringに安全に変換します（範囲ベース）
	/// </summary>
	/// <param name="u8str_view">変換元のUTF-8文字列ビュー</param>
	/// <returns>変換されたstd::string</returns>
	[[nodiscard(L"U8StringToStringSafe Not Used")]] inline std::string U8StringToStringSafe(const std::u8string_view& u8str_view)
	{
		return std::string(u8str_view.begin(), u8str_view.end());
	}

	/// <summary>
	/// typenameの型名を文字列として所得します
	/// </summary>
	/// <typeparam name="T">任意の型</typeparam>
	/// <returns>Tの型名の文字列</returns>
	template <typename T>
	[[nodiscard(L"WhatIsTypeName Not Used")]] inline const std::string WhatIsTypeName()
	{
		return typeid(T).name();
	}

	/// <summary>
	/// typenameの型名をハッシュコードとして所得します
	/// </summary>
	/// <typeparam name="T">任意の型</typeparam>
	/// <returns>Tの型名のハッシュコード</returns>
	template <typename T>
	[[nodiscard(L"WhatIsTypeName Not Used")]] inline const auto WhatIsTypeNameToHash()
	{
		return typeid(T).hash_code();
	}
}