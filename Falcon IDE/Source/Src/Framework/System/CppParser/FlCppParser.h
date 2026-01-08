#pragma once
#include <regex>

struct CppStructInfo {
    std::string name;
    std::vector<std::string> contentLines;
};

struct CppFunctionInfo {
    std::string name;
    std::string signature;
    std::vector<std::string> contentLines;
};

class FlSimpleCppParser
{
public:
    bool ParseFile(const std::string& path) noexcept;

    const std::vector<CppStructInfo>& GetStructs() const noexcept { return m_structs; }
    const std::vector<CppFunctionInfo>& GetFunctions() const noexcept { return m_functions; }

private:
    void Parse() noexcept;

    // 次の "{" を探す（行と位置）
    bool FindNextBrace(size_t startLine, size_t& outLine, size_t& outPos) noexcept;

    // 関数定義の検出
    void FindFunction(const std::string& line,const size_t index) noexcept;

    // `{` の位置から対応 `}` までのブロックを出力
    void ExtractScopeBlock(size_t braceLine, size_t bracePos, std::vector<std::string>& out) noexcept;

    std::vector<std::string> m_lines;

    // 結果
    std::vector<CppStructInfo> m_structs;
    std::vector<CppFunctionInfo> m_functions;
};
