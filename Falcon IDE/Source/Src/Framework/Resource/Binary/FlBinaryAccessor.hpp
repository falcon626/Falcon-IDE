#pragma once

class FlBinaryAccessor // If The File Is Not Open Return False
{
public:

	FlBinaryAccessor()  = default;
	~FlBinaryAccessor() = default;

	FlBinaryAccessor(const FlBinaryAccessor&)			 = delete;
	FlBinaryAccessor& operator=(const FlBinaryAccessor&) = delete;

	/// <summary>
	/// 指定されたファイル名にデータベクターをバイナリ形式で保存します。
	/// </summary>
	/// <typeparam name="T">ベクター内のデータ型。</typeparam>
	/// <param name="filename">保存先のファイル名（パスを含む）。</param>
	/// <param name="save">保存するデータのベクター。</param>
	/// <returns>保存に成功した場合は true、失敗した場合は false を返します。</returns>
	template <typename T>
	auto Save(_In_ const std::string_view& filename, _In_ const std::vector<T>& save) noexcept
	{
		auto file{ std::ofstream{filename.data(), std::ios::binary}};
		if (file.is_open())
		{
			// Save Data
			file.write(reinterpret_cast<const char*>(save.data()), save.size() * sizeof(T));
			file.close();
			return true;
		}
		else return false;
	}

	/// <summary>
	/// バイナリファイルからデータを読み込み、指定した型のベクターに格納します。
	/// </summary>
	/// <typeparam name="_T">ファイルから読み込むデータの型。</typeparam>
	/// <param name="filename">読み込むバイナリファイルのパスを表す文字列ビュー。</param>
	/// <param name="load">ファイルから読み込んだデータを格納する出力用のベクター。</param>
	/// <param name="elementsNum">読み込んだ要素数を格納する出力用の変数。</param>
	/// <returns>読み込みに成功した場合は true、失敗した場合は false を返します。</returns>
	template <typename _T>
	auto Load(_In_ const std::string_view& filename, _Out_ std::vector<_T>& load, _Out_ size_t& elementsNum) noexcept
	{
		// Initialize Output Parameters To Ensure They Are Not Left Uninitialized
		elementsNum = Def::ULongLongZero;
		load.clear();

		auto file{ std::ifstream{filename.data(), std::ios::binary}};
		if (file.is_open()) 
		{
			// Get File Size
			file.seekg(NULL, std::ios::end);
			auto size{ std::streamsize{file.tellg()} };
			file.seekg(NULL, std::ios::beg);

			// File Size Check
			if (size % sizeof(_T) != NULL) return false;

			// Load Data Element Count
			elementsNum = static_cast<size_t>(size) / sizeof(_T);

			// Load Data
			load.resize(elementsNum);
			if (file.read(reinterpret_cast<char*>(load.data()), size))
			{
				file.close();
				return true;
			}
			else
			{
				file.close();
				return false;
			}
		}
		else return false;
	}
};