#include "FlVisualStudioManager.h"
#include "FlCrypter/Src/FlCrypter.h"
import FlProcessCreater;
static bool WriteProjectTextFile(const std::filesystem::path& path, const std::string& text) noexcept
{
    return FlAssetProtector::WriteFileBinary(path, std::vector<uint8_t>(text.begin(), text.end()));
}

bool FlVisualStudioProjectManager::IsValidProjectName(const std::string& name) noexcept
{
	const auto isLetter = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };
	if (name.empty() || !isLetter(name.front()) || name.find("__") != std::string::npos) return false;
	if (!std::all_of(name.begin(), name.end(), [&](unsigned char c) {
		return isLetter(c) || (c >= '0' && c <= '9') || c == '_';
	})) return false;

	auto upper = name;
	for (auto& c : upper) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
	if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL") return false;
	return !(upper.size() == 4 && (upper.starts_with("COM") || upper.starts_with("LPT")) &&
		upper.back() >= '1' && upper.back() <= '9');
}

std::string FlVisualStudioProjectManager::CreateBuildCommand(const std::filesystem::path& projectPath,
	const std::string& configuration, const std::string& platform, const std::string& action)
{
	static const std::string vcvarsPath = [] {
		wchar_t programFiles[MAX_PATH]{};
		const auto length = GetEnvironmentVariableW(L"ProgramFiles(x86)", programFiles, MAX_PATH);
		if (!length || length >= MAX_PATH) return std::string{};
		const auto vswhere = std::filesystem::path(programFiles) / L"Microsoft Visual Studio/Installer/vswhere.exe";
		std::string output;
		DWORD exitCode{};
		if (!::ExecuteCommand(L"\"" + vswhere.wstring() +
			L"\" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath -utf8",
			output, exitCode) || exitCode != 0) return std::string{};
		const auto end = output.find_first_of("\r\n");
		output.resize(end == std::string::npos ? output.size() : end);
		if (output.empty()) return std::string{};
		const auto path = std::filesystem::path(utf8_to_wide(output)) / L"VC/Auxiliary/Build/vcvarsall.bat";
		std::error_code ec;
		return std::filesystem::is_regular_file(path, ec) && !ec ? wide_to_ansi(path.wstring()) : std::string{};
	}();
	if (vcvarsPath.empty()) return {};
	const auto architecture = platform == "Win32" ? "x86" : "x64";
	const auto solutionDir = std::filesystem::absolute(std::filesystem::current_path()).string() + "\\\\";
	return "call \"" + vcvarsPath + "\" " + architecture + " && msbuild \"" +
		std::filesystem::absolute(projectPath).lexically_normal().string() +
		"\" /p:Configuration=" + configuration + " /p:Platform=" + platform +
		" /p:SolutionDir=\"" + solutionDir + "\" /t:" + action;
}

bool FlVisualStudioProjectManager::CreateNewProject(const std::string& projectName, const std::filesystem::path& targetDir) noexcept
{
    if (!IsValidProjectName(projectName))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Invalid project name: %s", projectName.c_str());
        return false;
    }
    const auto projDir = targetDir / projectName;
    const auto scriptDir = std::filesystem::path("ScriptModule") / projectName;
    std::error_code ec;
    const auto projectExists = std::filesystem::exists(projDir, ec);
    if (ec) return false;
    const auto scriptExists = std::filesystem::exists(scriptDir, ec);
    if (ec || projectExists || scriptExists)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Project or script already exists: %s", projectName.c_str());
        return false;
    }

    const auto stageRoot = targetDir / (".FlCreate-" + FlGuid{}.ToString());
    const auto stageProject = stageRoot / "Project";
    const auto stageScript = stageRoot / "Script";
    const auto stageSolution = stageRoot / ("Solution" + m_solutionPath.extension().string());
    bool projectPublished = false;
    bool scriptPublished = false;
    const auto failed = [&] {
        if (scriptPublished)
        {
            std::filesystem::rename(scriptDir, stageScript, ec);
            if (ec) FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Could not restore script staging; files remain at %s", scriptDir.string().c_str());
        }
        if (projectPublished)
        {
            std::filesystem::rename(projDir, stageProject, ec);
            if (ec) FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Could not restore project staging; files remain at %s", projDir.string().c_str());
        }
        // ponytail: retain failed staging and any user-added files for inspection; prune when disk usage matters.
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Project creation failed; staged files retained at %s", stageRoot.string().c_str());
        return false;
    };
    try {
        std::vector<uint8_t> originalSolution;
        if (!FlAssetProtector::ReadFileBinary(m_solutionPath, originalSolution)) return false;
        std::filesystem::create_directories(targetDir);
        if (!std::filesystem::create_directory(stageRoot)) return false;
        std::filesystem::create_directory(stageProject);
        if (!CreateSourceFiles(stageProject, projectName, stageScript) ||
            !CreateVcxproj(stageProject, projectName) || !CreateFilters(stageProject, projectName) ||
            !FlAssetProtector::WriteFileBinary(stageSolution, originalSolution)) return failed();

        const auto projFile = projDir / (projectName + ".vcxproj");
        if (m_solutionPath.extension() == ".sln")
        {
            if (!AddProjectToSolution(stageSolution, projFile, projectName)) return failed();
        }
        else if (m_solutionPath.extension() == ".slnx")
        {
            if (!AddProjectToSolutionSlnx(stageSolution, projFile)) return failed();
        }
        else return failed();

        std::vector<uint8_t> generatedSolution, currentSolution;
        if (!FlAssetProtector::ReadFileBinary(stageSolution, generatedSolution) ||
            !FlAssetProtector::ReadFileBinary(m_solutionPath, currentSolution) || currentSolution != originalSolution)
            return failed();
        std::filesystem::create_directories(scriptDir.parent_path());
        std::filesystem::rename(stageProject, projDir);
        projectPublished = true;
        std::filesystem::rename(stageScript, scriptDir);
        scriptPublished = true;
        if (!FlAssetProtector::WriteFileBinary(m_solutionPath, generatedSolution)) return failed();
        std::filesystem::remove(stageSolution, ec);
        std::filesystem::remove(stageRoot, ec);
        FlEditorAdministrator::Instance().GetLogger()->AddLog("Project '%s' created & added to solution.", projectName.c_str());
        return true;
    }
    catch (...) {
        return failed();
    }
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

bool FlVisualStudioProjectManager::CreateSourceFiles(const std::filesystem::path& dir,
    const std::string& name, const std::filesystem::path& scriptDir) noexcept
{
    try {
        std::filesystem::create_directories(dir / "Src");
        if (!std::filesystem::create_directory(scriptDir)) return false;
        const auto sourceTemplate = LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.cxx.flsample");
        if (sourceTemplate.empty()) return false;
        const auto source = Str::ReplaceString(sourceTemplate, "#ProjectName#", name);
        return WriteProjectTextFile(dir / "Src/Pch.cc", "#include \"Pch.h\"") &&
            WriteProjectTextFile(scriptDir / (name + ".cxx"), source);
    }
    catch (...) { return false; }
}

bool FlVisualStudioProjectManager::CreateVcxproj(const std::filesystem::path& dir, const std::string& name) noexcept
{
    const auto source = LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.vcxproj.flsample");
    if (source.empty()) return false;
    const auto named = Str::ReplaceString(source, "#ProjectName#", name);
    return WriteProjectTextFile(dir / (name + ".vcxproj"), Str::ReplaceString(named, "#GUID#", FlGuid{}.ToString()));
}

bool FlVisualStudioProjectManager::CreateFilters(const std::filesystem::path& dir, const std::string& name) noexcept
{
    const auto source = LoadTextFile("Src/Framework/System/VisualStudioManager/Sample/Template.vcxproj.filters.flsample");
    if (source.empty()) return false;
    const auto named = Str::ReplaceString(source, "#ProjectName#", name);
    return WriteProjectTextFile(dir / (name + ".vcxproj.filters"), Str::ReplaceString(named, "#GUID#", FlGuid{}.ToString()));
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
	ifs.close();

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
    std::string output;
    for (const auto& lineText : lines) output += lineText + "\n";
    return WriteProjectTextFile(sln, output);
}


bool FlVisualStudioProjectManager::AddProjectToSolutionSlnx(const std::filesystem::path& slnx, const std::filesystem::path& projPath) noexcept
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(slnx.string().c_str()) != tinyxml2::XML_SUCCESS)
		return false;

	auto* root = doc.FirstChildElement("Solution");
	if (!root) return false;

	FlGuid guid;

	// 新しい Project ノードを作成
	auto* newProj = doc.NewElement("Project");
	newProj->SetAttribute("Path", projPath.generic_string().c_str());
	newProj->SetAttribute("Id", guid.ToString().c_str());

	// Solution の末尾に追加
	root->InsertEndChild(newProj);

	// 保存
	tinyxml2::XMLPrinter printer;
	doc.Print(&printer);
	return WriteProjectTextFile(slnx, printer.CStr());
}

bool FlVisualStudioProjectManager::RemoveProjectFromSolutionSlnx(const std::filesystem::path& slnx, const std::filesystem::path& projPath) noexcept
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(slnx.string().c_str()) != tinyxml2::XML_SUCCESS)
		return false;

	auto* root = doc.FirstChildElement("Solution");
	if (!root) return false;

	// 全プロジェクトを走査して一致する Path を削除
	for (auto* elem = root->FirstChildElement("Project"); elem; )
	{
		auto* next = elem->NextSiblingElement("Project");

		const char* pathAttr = elem->Attribute("Path");
		if (pathAttr && projPath.generic_string() == pathAttr)
			root->DeleteChild(elem);
		elem = next;
	}

	// 保存
	tinyxml2::XMLPrinter printer;
	doc.Print(&printer);
	return WriteProjectTextFile(slnx, printer.CStr());
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
	// (0) .slnxであればXML操作で削除
	// ----------------------------------------------------
	if (m_solutionPath.extension().string() == ".slnx")
	{
		if (!RemoveProjectFromSolutionSlnx(m_solutionPath, projPath)) return false;
		FlEditorAdministrator::Instance().GetLogger()->AddLog(
			"Removed project (%s) from solution.", projPath.filename().string().c_str()
		);
	}
	else if (m_solutionPath.extension().string() == ".sln")
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
	}

	// ----------------------------------------------------
	// (6) ヘッダインクルード文削除
	// ----------------------------------------------------
	auto projName{ projPath.parent_path().filename().string() };
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

	// ----------------------------------------------------
	// (7) コードファイル削除
	// ----------------------------------------------------
	const std::filesystem::path scriptDir{ "ScriptModule" };
	auto scriptPath{ scriptDir / projName };

	if (!std::make_unique<FlFileWatcher>()->RemDirectory(scriptPath))
	{
		FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed Remove ScriptModule directory %s", scriptPath.string().c_str());
		return false;
	}

	return true;
}