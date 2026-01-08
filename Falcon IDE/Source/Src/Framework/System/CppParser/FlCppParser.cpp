#include "FlCppParser.h"

bool FlSimpleCppParser::ParseFile(const std::string& path) noexcept
{
    std::ifstream ifs(path);
    if (!ifs) return false;

    m_lines.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        m_lines.push_back(line);
    }

    Parse();
    return true;
}

void FlSimpleCppParser::Parse() noexcept
{
    m_structs.clear();
    m_functions.clear();

    auto scope{ Def::IntZero };
    for (auto i{ Def::ULongLongZero }; i < m_lines.size(); ++i)
    {
        const auto& l{ m_lines[i] };

        // --- スコープ外（scope==0）のときに構造体や関数を検出する ---
        if (scope == Def::IntZero)
        {
            // struct / class / enum の検出
            {
                const std::regex re(R"REGEX((struct|class|enum)\s+([A-Za-z_]\w*))REGEX");

                std::smatch m;
                if (std::regex_search(l, m, re))
                {
                    // 次の "{" を探す
                    size_t braceLine;
                    size_t bracePos;
                    if (FindNextBrace(i, braceLine, bracePos))
                    {
                        CppStructInfo info;
                        info.name = m[2].str();
                        ExtractScopeBlock(braceLine, bracePos, info.contentLines);
                        m_structs.push_back(info);
                    }
                }
            }

            // 関数定義の検出
            FindFunction(l, i);
        }

        // --- スコープの変動を解析 ---
        scope += Str::CountChar(l, '{');
        scope -= Str::CountChar(l, '}');
    }
}

bool FlSimpleCppParser::FindNextBrace(size_t startLine, size_t& outLine, size_t& outPos) noexcept
{
    for (auto i{ startLine }; i < m_lines.size(); ++i)
    {
        auto pos{ m_lines[i].find("{") };
        if (pos != std::string::npos) {
            outLine = i;
            outPos = pos;
            return true;
        }
    }
    return false;
}

static std::string ExtractFunctionName(const std::string& line)
{
    // "(" の位置
    size_t posParen = line.find("(");
    if (posParen == std::string::npos) return "";

    // "(" より前の部分を抽出
    std::string beforeParen = line.substr(0, posParen);

    // 行頭・末尾の空白除去
    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
    trim(beforeParen);

    // 最後の空白を探す → その後ろが関数名
    size_t lastSpace = beforeParen.find_last_of(" \t");
    if (lastSpace == std::string::npos)
    {
        // 空白が無い → そのまま関数名？
        return beforeParen;
    }

    return beforeParen.substr(lastSpace + 1);
}

void FlSimpleCppParser::FindFunction(const std::string& line, const size_t index) noexcept
{
    if (line.find("(") != std::string::npos &&
        line.find(")") != std::string::npos &&
        line.find(";") == std::string::npos)
    {
        std::string signature = line;

        // --- 次の "{" を探す ---
        size_t braceLine;
        size_t bracePos;
        if (FindNextBrace(index, braceLine, bracePos))
        {
            // 「)」より後に「{」がある or 次行にある場合のみ関数確定
            if (braceLine == index)
            {
                // 同行に { がある
                if (bracePos > line.find(")"))
                {
                    CppFunctionInfo info;
                    info.name = ExtractFunctionName(signature);
                    info.signature = signature;
                    ExtractScopeBlock(braceLine, bracePos, info.contentLines);
                    m_functions.push_back(info);
                }
            }
            else
            {
                // { が次の行以降にある（OK）
                CppFunctionInfo info;
                info.name = ExtractFunctionName(signature);
                info.signature = signature;
                ExtractScopeBlock(braceLine, bracePos, info.contentLines);
                m_functions.push_back(info);
            }
        }
    }
}

void FlSimpleCppParser::ExtractScopeBlock(size_t braceLine, size_t bracePos, std::vector<std::string>& out) noexcept
{
    auto scope{ Def::IntZero };
    auto started{ false };

    for (auto i{ braceLine }; i < m_lines.size(); ++i) {
        const auto& l{ m_lines[i] };

        // スタート地点
        if (!started)
        {
            if (i == braceLine) 
            {
                scope += Def::IntOne; // 最初の "{"
                started = true;
            }
            continue;
        }

        // ブロック開始後の処理
        scope += Str::CountChar(l, '{');
        scope -= Str::CountChar(l, '}');

        out.push_back(l);

        if (scope == Def::IntZero) return;
    }
}
