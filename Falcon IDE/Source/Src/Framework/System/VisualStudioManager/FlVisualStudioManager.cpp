#include "FlVisualStudioManager.h"
import FlProcessCreater;

bool FlVisualStudioProjectManager::CreateNewProject(const std::string& projectName, const std::filesystem::path& targetDir) noexcept
{
	auto projDir = targetDir / projectName;

	// ---------------------------------------------------
	// (1) ディレクトリ作成
	// ---------------------------------------------------
	try {
		std::filesystem::create_directories(projDir);
	}
	catch (...) {
		FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to create directory: %s", projDir.string().c_str());
		return false;
	}

	// ---------------------------------------------------
	// (2) 初期コードファイル生成
	// ---------------------------------------------------
	if (!CreateSourceFiles(projDir, projectName)) {
		return false;
	}

	// ---------------------------------------------------
	// (3) .vcxproj と .filters を生成
	// ---------------------------------------------------
	if (!CreateVcxproj(projDir, projectName)) return false;
	if (!CreateFilters(projDir, projectName)) return false;

	// ---------------------------------------------------
	// (4) .sln に Project を追加
	// ---------------------------------------------------
	auto projFile = projDir / (projectName + ".vcxproj");
	if (!AddProjectToSolution(m_solutionPath, projFile, projectName)) {
		return false;
	}

	FlEditorAdministrator::Instance().GetLogger()->AddLog("Project '%s' created & added to solution.", projectName.c_str());
	return true;
}

static const std::string LoadTextFile(const std::filesystem::path& path) noexcept
{
    if (!std::filesystem::exists(path)) return "";

    std::ifstream ifs(path);
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

bool FlVisualStudioProjectManager::FormingModule(const std::filesystem::path& projDir, const std::filesystem::path& codeFile) noexcept
{
    m_cppParser->ParseFile(codeFile.string());
    m_autoAdd->SetProjectDirPath(projDir);

    auto projName{ projDir.filename().string()};
    auto srt{ std::string{} }; // 構造体コード

    for (auto& s : m_cppParser->GetStructs())
    {
        if (!Str::Contains(s.name, "Component")) continue;

        const auto reBuildStruct{
            [](const std::string& name, const std::vector<std::string>& lines) noexcept
            {
                auto result{ std::string{
                    "#pragma once\ntypedef struct " + name + '\n' +
                    "{\n"
                } };

                for (auto& l : lines)
                    result += l + '\n';
                return result;
            }
        };
        srt = reBuildStruct(s.name, s.contentLines);
    }

    auto temp{ LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.c++.flsample") };

    for (auto& fun : m_cppParser->GetFunctions())
    {
        // 実装ファイル自動生成
        // 作成された実装ファイル内で必要コード自動生成

        const auto reBuildFunction{
            [](const std::vector<std::string>& lines) noexcept
            {
                auto result{ std::string{} };

                for (auto l : lines)
                {       
                    result += "                     " + l + '\n';

                    auto ps{ result.find("//") };
                    if (ps != std::string::npos) result.erase(ps);
                }
                
                result.pop_back();
                result.pop_back();
                return result;
            }
        };
        if (Str::Contains(fun.name, "Start"))
        {
            auto s{ reBuildFunction(fun.contentLines) };
            temp = Str::ReplaceString(temp, "#ImpStart#", s);
        }
        else if (Str::Contains(fun.name, "OnDestroy"))
        {
            auto s{ reBuildFunction(fun.contentLines) };
            temp = Str::ReplaceString(temp, "#ImpOnDestroy#", s);
        }
        else if (Str::Contains(fun.name, "Serialize"))
        {
            auto s{ reBuildFunction(fun.contentLines) };
            temp = Str::ReplaceString(temp, "#ImpSerialize#", s);
        }
        else if (Str::Contains(fun.name, "Deserialize"))
        {
            auto s{ reBuildFunction(fun.contentLines) };
            temp = Str::ReplaceString(temp, "#ImpDeserialize#", s);
        }
        else if (Str::Contains(fun.name, "RenderEditor"))
        {
            auto s{ reBuildFunction(fun.contentLines) };
            temp = Str::ReplaceString(temp, "#ImpRenderEditor#", s);
        }
        else if (Str::Contains(fun.name, "Update"))
        {
            auto s{ reBuildFunction(fun.contentLines) };
            temp = Str::ReplaceString(temp, "#ImpUpdate#", s);
        }
    }
    temp = Str::ReplaceString(temp, "#ProjectName#", projName);

    auto autoGenDir{ std::string{"Src/AutomaticallyGenerated"} };
    auto compHeaderPath{ projName + "ComponentAutomaticallyGenerated.hh" };

    if (m_autoAdd->AddFiles(
        { {projName + "AutomaticallyGenerated.c++", temp}, {compHeaderPath, srt} }
        , autoGenDir))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddLog("Added automatically generated files to project %s", projName.c_str());

        auto relativePath{
            "../" +
            projName + "/" +
            autoGenDir + "/" +
            compHeaderPath
        };

        auto dyLib{ projDir.parent_path() };

        auto filePath{ dyLib / "Common/FlComponentGroup.hpp" };

        {
            std::ifstream ifs(filePath);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

            if (content.find(relativePath) != std::string::npos) return true;
        }

        auto ok{ Str::ReplaceStringInFile(
            filePath,
            "// <!--NextEntry-->",
            Str::ReplaceString(Str::ConvertToCRLF(
                R"IXX(#include "#HeaderPath#"
// <!--NextEntry-->)IXX"
         ), "#HeaderPath#", relativePath))
        };

        if (ok)
        {
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Add FlComponentGroup.hpp for project %s", projName.data());
            return true;
        }
        else
        {
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to add module entry to FlComponentGroup.hpp for project %s", projName.data());
            return false;
        }
    }
    else
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to add automatically generated files to project %s", projName.c_str());
        return false;
    }
}

bool FlVisualStudioProjectManager::CreateSourceFiles(const std::filesystem::path& dir, const std::string& name) noexcept
{
    const std::string srcDir{ "Src" };
    const std::filesystem::path scriptDir{ "ScriptModule" };

    try {
        std::filesystem::create_directories(dir / srcDir);
        std::filesystem::create_directories(scriptDir / name);
    }
    catch (...) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to create directory: %s", srcDir.c_str());
        return false;
    }
    auto pchPath{ dir / srcDir / ("Pch.cc") };
    auto cppPath{ scriptDir / name / (name + ".cxx") };

    // --- Pch ---
    std::ofstream ofsPch(pchPath);
    if (!ofsPch) return false;

    // Custom Delimiter
    ofsPch << R"PCH(#include "Pch.h")PCH";

    // --- Source ---
    {
        std::ofstream ofs(cppPath);
        if (!ofs) return false;

        // Custom Delimiter
        ofs << LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.cxx.flsample");
    } // ←close

    ReplaceInFile(cppPath, "#ProjectName#", name);

    return true;
}

bool FlVisualStudioProjectManager::CreateVcxproj(const std::filesystem::path& dir, const std::string& name) noexcept
{
    auto path = dir / (name + ".vcxproj");

    {
        std::ofstream ofs(path);
        if (!ofs) return false;

        ofs << LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.vcxproj.flsample");
    } // ←close

    ReplaceInFile(path, "#ProjectName#", name);
    ReplaceInFile(path, "#GUID#");

    return true;
}

bool FlVisualStudioProjectManager::CreateFilters(const std::filesystem::path& dir, const std::string& name) noexcept
{
    auto path = dir / (name + ".vcxproj.filters");

    {
        std::ofstream ofs(path);
        if (!ofs) return false;

        auto str = LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.vcxproj.filters.flsample");
        ofs << str;
    } // ←close

    ReplaceInFile(path, "#ProjectName#", name);
    ReplaceInFile(path, "#GUID#");

    return true;
}

bool FlVisualStudioProjectManager::AddToSolutionUsingDotNet(const std::filesystem::path& sln, const std::filesystem::path& vcxproj) noexcept
{
    std::wstring wcmd {
        L"dotnet sln \"" +
        std::filesystem::absolute(sln).wstring() +
        L"\" add \"" +
        std::filesystem::absolute(vcxproj).wstring() +
        L"\"" };
   
    std::string stdOutput{};
    DWORD exitCode{};

    auto isLaunched{ ExecuteCommand(wcmd, stdOutput, exitCode) };

    if (!isLaunched)
    {
        // プロセス自体の起動に失敗した場合 (dotnet コマンドが見つからないなど)
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Fatal: Failed to start dotnet.");
        return false;
    }
    else if (exitCode == Def::ULongZero)
    {
        // 成功 (ExitCode 0)
        FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Add Project");

        if (!stdOutput.empty()) 
            FlEditorAdministrator::Instance().GetLogger()->AddLogU8(u8"%s", stdOutput);
    }
    else
    {
        // 失敗 (ExitCode が 0 以外)
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Faild Add Project  code: %lu", exitCode);
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLogU8(u8"Detail: %s", stdOutput.c_str());
        return false;
    }
    return true;
}

bool FlVisualStudioProjectManager::AddProjectToSolution(
    const std::filesystem::path& sln,
    const std::filesystem::path& projPath,
    const std::string& name
) noexcept
{
    // 読み込み
    std::ifstream ifs(sln);
    if (!ifs) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        lines.push_back(line);
    };

    FlGuid guid{};
    auto guidStr{ guid.ToString() };

    const std::string cppProjectTypeGuid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
    std::string projectGuid = '{' + guidStr + '}';

    // -----------------------------
    // ① Project セクション挿入
    // -----------------------------
    const std::string projectHeader =
        "Project(\"" + cppProjectTypeGuid + "\") = \"" + name + "\", \"" +
        projPath.string() + "\", \"" + projectGuid + "\"";

    const std::string projectFooter = "EndProject";

    // "EndProject" の最後の直後に追加する
    size_t insertProjectIndex = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find("EndProject") != std::string::npos) {
            insertProjectIndex = i + 1;
        }
    }

    lines.insert(lines.begin() + insertProjectIndex, projectFooter);
    lines.insert(lines.begin() + insertProjectIndex, projectHeader);

    // -----------------------------
    // ② ProjectConfigurationPlatforms 挿入
    // -----------------------------
    const char* configs[] = {
        "Debug|x64", "Debug|x86", "Debug|Any CPU",
        "Release|x64", "Release|x86", "Release|Any CPU"
    };

    // 見つける
    size_t configSectionStart = 0;
    size_t configSectionEnd = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find("GlobalSection(ProjectConfigurationPlatforms)") != std::string::npos) {
            configSectionStart = i;
        }
        if (configSectionStart > 0 && lines[i].find("EndGlobalSection") != std::string::npos) {
            configSectionEnd = i;
            break;
        }
    }

    if (configSectionStart == 0) return false;

    // 挿入位置 = "EndGlobalSection" の行
    size_t insertConfigIndex = configSectionEnd;

    // 設定追加
    for (auto& cfg : configs) {
        std::string active = "        " + projectGuid + "." + cfg + ".ActiveCfg = " + cfg;
        std::string build = "        " + projectGuid + "." + cfg + ".Build.0 = " + cfg;

        lines.insert(lines.begin() + insertConfigIndex, build);
        lines.insert(lines.begin() + insertConfigIndex, active);

        insertConfigIndex += 2;
    }

    // -----------------------------
    // 書き戻し
    // -----------------------------
    std::ofstream ofs(sln);
    if (!ofs) return false;

    for (auto& l : lines) {
        ofs << l << "\n";
    }

    return true;
}


void FlVisualStudioProjectManager::ReplaceInFile(const std::filesystem::path& path, const std::string& from, const std::string& to) noexcept
{
    std::ifstream ifs(path);
    std::stringstream buffer;
    buffer << ifs.rdbuf();

    std::string text{ buffer.str() };
    size_t pos{ Def::ULongLongZero };

    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }

    std::ofstream ofs(path);
    ofs << text;
}

void FlVisualStudioProjectManager::ReplaceInFile(const std::filesystem::path& path, const std::string& from) noexcept
{
    std::ifstream ifs(path);
    std::stringstream buffer;
    buffer << ifs.rdbuf();

    std::string text{ buffer.str() };
    size_t pos{ Def::ULongLongZero };

    while ((pos = text.find(from, pos)) != std::string::npos) {

        // GUID生成
        FlGuid guid;
        auto guidStr{ guid.ToString() };

        text.replace(pos, from.size(), guidStr);
        pos += guidStr.size();
    }

    std::ofstream ofs(path);
    ofs << text;
}

bool FlVisualStudioProjectManager::RemoveProjectFromSolution(const std::filesystem::path& projPath) noexcept
{
    // ----------------------------------------------------
    // (1) ソリューション読み込み
    // ----------------------------------------------------
    std::ifstream ifs(m_solutionPath);
    if (!ifs) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);

    ifs.close();

    // ----------------------------------------------------
    // (2) 削除対象プロジェクト GUID を取得
    // ----------------------------------------------------
    // Project("GUID") = "Name", "path", "{TARGET_GUID}"
    std::string projFilename = projPath.filename().string();

    size_t projStart = std::string::npos;
    size_t projEnd = std::string::npos;
    std::string targetGUID;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].find("Project(") != std::string::npos &&
            lines[i].find(projFilename) != std::string::npos)
        {
            projStart = i;

            // GUID を抽出
            auto pos = lines[i].rfind('{');
            if (pos != std::string::npos)
                targetGUID = lines[i].substr(pos);

            // EndProject を探す
            for (size_t j = i + 1; j < lines.size(); ++j)
            {
                if (lines[j].find("EndProject") != std::string::npos)
                {
                    projEnd = j;
                    break;
                }
            }
            break;
        }
    }

    if (projStart == std::string::npos || projEnd == std::string::npos)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(
            "RemoveProject: Project entry not found in .sln (%s)", projFilename.c_str()
        );
        return false;
    }

    // ----------------------------------------------------
    // (3) Project ブロック削除
    // ----------------------------------------------------
    lines.erase(lines.begin() + projStart, lines.begin() + projEnd + 1);

    // ----------------------------------------------------
    // (4) GlobalSection(ProjectConfigurationPlatforms) の削除処理
    // ----------------------------------------------------
    size_t configStart = std::string::npos;
    size_t configEnd = std::string::npos;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].find("GlobalSection(ProjectConfigurationPlatforms)") != std::string::npos)
            configStart = i;

        if (configStart != std::string::npos &&
            lines[i].find("EndGlobalSection") != std::string::npos)
        {
            configEnd = i;
            break;
        }
    }

    if (configStart != std::string::npos && configEnd != std::string::npos)
    {
        std::vector<std::string> newConfig;
        newConfig.reserve(configEnd - configStart + 1);

        for (size_t i = configStart + 1; i < configEnd; ++i)
        {
            if (lines[i].find(targetGUID) == std::string::npos)
                newConfig.push_back(lines[i]);
        }

        // 元の領域を差し替え
        lines.erase(lines.begin() + configStart + 1, lines.begin() + configEnd);
        lines.insert(lines.begin() + configStart + 1, newConfig.begin(), newConfig.end());
    }

    // ----------------------------------------------------
    // (5) 書き戻し
    // ----------------------------------------------------
    std::ofstream ofs(m_solutionPath);
    if (!ofs) return false;

    for (auto& l : lines)
        ofs << l << "\n";

    ofs.close();

    FlEditorAdministrator::Instance().GetLogger()->AddLog(
        "Removed project (%s) from solution.", projFilename.c_str()
    );

    const std::filesystem::path scriptDir{ "ScriptModule" };
    auto projName{ projPath.parent_path().filename().string()};
    auto scriptPath{ scriptDir / projName };

    if (!std::make_unique<FlFileWatcher>()->RemDirectory(scriptPath))
    {
       FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed Remove ScriptModule directory %s", scriptPath.string().c_str());
       return false;
    }

    auto compHeader{ "../" + projName +
        "/Src/AutomaticallyGenerated/" +
        projName + "ComponentAutomaticallyGenerated.hh" };

    auto compGroupPath = std::filesystem::path("DynamicLib/Common/FlComponentGroup.hpp");
    {
        std::ifstream ifs2(compGroupPath);
        if (!ifs2)
        {
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Cannot open FlComponentGroup.hpp");
            return false;
        }

        std::vector<std::string> cgLines;
        std::string l2;
        while (std::getline(ifs2, l2))
            cgLines.push_back(l2);

        std::vector<std::string> cgOut;

        for (auto& l : cgLines)
        {
            if (l.find(compHeader) == std::string::npos)
                cgOut.push_back(l);
        }

        std::ofstream ofs2(compGroupPath);
        for (auto& x : cgOut)
            ofs2 << x << "\n";
    }

    return true;
}