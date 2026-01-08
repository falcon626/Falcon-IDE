#include "FlAutomaticFileAddSystem.h"

using namespace tinyxml2;

FlAutomaticFileAddSystem::FlAutomaticFileAddSystem(const std::filesystem::path& vcxprojPath,
    const std::filesystem::path& filtersPath)
    : m_vcxprojPath{ vcxprojPath }, m_filtersPath{ filtersPath }, m_projectDir{ m_vcxprojPath.parent_path().lexically_normal() },
    TypeMap{
        // コンパイル対象
        { ".c", "ClCompile" }, { ".cc", "ClCompile" }, { ".cpp", "ClCompile" }, { ".cxx", "ClCompile" }, { ".c++", "ClCompile" },
        { ".ixx", "ClCompile" }, // C++20 Modules

        // ヘッダー系
        { ".h", "ClInclude" }, { ".hh", "ClInclude" }, { ".hpp", "ClInclude" }, { ".hxx", "ClInclude" }, { ".h++", "ClInclude" },
        { ".inl", "ClInclude" },

        // その他
        { ".rc", "ResourceCompile" },
        { ".idl", "Midl" },
        { ".asm", "MASM" },
        { ".hlsl", "FxCompile" },
    }
{}

FlAutomaticFileAddSystem::FlAutomaticFileAddSystem()
    : TypeMap{
        // コンパイル対象
        { ".c", "ClCompile" }, { ".cc", "ClCompile" }, { ".cpp", "ClCompile" }, { ".cxx", "ClCompile" }, { ".c++", "ClCompile" },
        { ".ixx", "ClCompile" }, // C++20 Modules

        // ヘッダー系
        { ".h", "ClInclude" }, { ".hh", "ClInclude" }, { ".hpp", "ClInclude" }, { ".hxx", "ClInclude" }, { ".h++", "ClInclude" },
        { ".inl", "ClInclude" },

        // その他
        { ".rc", "ResourceCompile" },
        { ".idl", "Midl" },
        { ".asm", "MASM" },
        { ".hlsl", "FxCompile" },
    }
{}

bool FlAutomaticFileAddSystem::AddFiles(const std::vector<FileCreationConfig>& files, const std::string& filterPath) noexcept
{
	if (files.empty()) return true;

	auto normalizedFilterPath{ std::filesystem::path(filterPath).lexically_normal().string() };

	// 出力先ディレクトリの作成
	auto outputDir{ m_projectDir / normalizedFilterPath };
	if (!std::filesystem::exists(outputDir))
	{
		try {
			std::filesystem::create_directories(outputDir);
		}
		catch (...) {
			return false;
		}
	}

	// 1. 実ファイルの書き込み
	for (const auto& file : files)
	{
		auto fullPath = outputDir / file.fileName;
		if (!WriteFileToDisk(fullPath, file.content)) return false;
	}

	// 2. vcxprojの更新 (一度だけLoad/Save)
	{
		tinyxml2::XMLDocument doc;
		if (doc.LoadFile(m_vcxprojPath.string().c_str()) != XML_SUCCESS) return false;

		for (const auto& file : files)
		{
			auto fullPath = outputDir / file.fileName;
			if (file.itemType == Def::EmptyStr) AddToVcxproj(doc, fullPath, DetectItemType(file.fileName));
			else AddToVcxproj(doc, fullPath, file.itemType);
		}

		if (doc.SaveFile(m_vcxprojPath.string().c_str()) != XML_SUCCESS) return false;
	}

	// 3. filtersの更新 (一度だけLoad/Save)
	{
		tinyxml2::XMLDocument doc;
		if (doc.LoadFile(m_filtersPath.string().c_str()) != XML_SUCCESS) return false;

		// 指定されたフィルタ階層が存在しない場合は作成
		EnsureFilterPathExists(doc, normalizedFilterPath);

		for (const auto& file : files)
		{
			auto fullPath = outputDir / file.fileName;
			if (file.itemType == Def::EmptyStr) AddToFilters(doc, fullPath, DetectItemType(file.fileName), normalizedFilterPath);
			else AddToFilters(doc, fullPath, file.itemType, normalizedFilterPath);
		}

		if (doc.SaveFile(m_filtersPath.string().c_str()) != XML_SUCCESS) return false;
	}

    return true;
}

bool FlAutomaticFileAddSystem::RemoveFiles(const std::vector<std::string>& fileNames, const std::string& filterPath) noexcept
{
    auto normalizedFilterPath{ std::filesystem::path(filterPath).lexically_normal().string() };
    auto outputDir{ m_projectDir / normalizedFilterPath };

    // vcxprojから削除
    {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(m_vcxprojPath.string().c_str()) == XML_SUCCESS)
        {
            for (const auto& name : fileNames)
            {
                RemoveFromVcxproj(doc, outputDir / name);
            }
            doc.SaveFile(m_vcxprojPath.string().c_str());
        }
    }

    // filtersから削除
    {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(m_filtersPath.string().c_str()) == XML_SUCCESS)
        {
            for (const auto& name : fileNames)
            {
                RemoveFromFilters(doc, outputDir / name);
            }
            doc.SaveFile(m_filtersPath.string().c_str());
        }
    }

    // 実ファイルの削除
    bool allDeleted = true;
    for (const auto& name : fileNames)
    {
        if (!DeleteFileFromDisk(outputDir / name))
            allDeleted = false;
    }

    return allDeleted;
}

void FlAutomaticFileAddSystem::SetProjectDirPath(std::filesystem::path projectDir) noexcept
{
    // <QuickReturn:存在しない場合>
    if (!std::filesystem::exists(projectDir)) return;

    m_projectDir  = projectDir.lexically_normal();
    m_vcxprojPath = *Str::GetFilePaths(m_projectDir.string(), ".vcxproj").data();
    m_filtersPath = *Str::GetFilePaths(m_projectDir.string(), ".filters").data();
}

bool FlAutomaticFileAddSystem::WriteFileToDisk(const std::filesystem::path& fullPath, const std::string& content) noexcept
{
    std::ofstream ofs(fullPath);
    if (!ofs) return false;
    ofs << content;
    return true;
}

bool FlAutomaticFileAddSystem::DeleteFileFromDisk(const std::filesystem::path& fullPath) noexcept
{
    try {
        if (std::filesystem::exists(fullPath))
            return std::filesystem::remove(fullPath);
        return true; // 存在しないなら成功扱い
    }
    catch (...) {
        return false;
    }
}

// ---------------------------------------------------------
// XML Manipulation Helper
// ---------------------------------------------------------

bool FlAutomaticFileAddSystem::AddToVcxproj(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath, const std::string& itemType) noexcept
{
    XMLElement* root = doc.RootElement();
    if (!root) return false;

    // ItemGroupを探す、なければ作る
    XMLElement* itemGroup = nullptr;
    for (XMLElement* ig = root->FirstChildElement("ItemGroup"); ig; ig = ig->NextSiblingElement("ItemGroup"))
    {
        if (ig->FirstChildElement(itemType.c_str()))
        {
            itemGroup = ig;
            break;
        }
    }
    if (!itemGroup)
    {
        itemGroup = doc.NewElement("ItemGroup");
        root->InsertEndChild(itemGroup);
    }

    auto relPath = std::filesystem::relative(filePath, m_projectDir).generic_string();

    // 重複チェック (簡易)
    for (XMLElement* e = itemGroup->FirstChildElement(itemType.c_str()); e; e = e->NextSiblingElement(itemType.c_str()))
    {
        const char* inc = e->Attribute("Include");
        if (inc && relPath == inc) return true; // 既にある
    }

    XMLElement* elem = doc.NewElement(itemType.c_str());
    elem->SetAttribute("Include", relPath.c_str());
    itemGroup->InsertEndChild(elem);

    return true;
}

void FlAutomaticFileAddSystem::EnsureFilterPathExists(tinyxml2::XMLDocument& doc, const std::string& filterPathStr) noexcept
{
    XMLElement* root = doc.RootElement();
    if (!root) return;

    std::string filterPathWin = filterPathStr;
    std::replace(filterPathWin.begin(), filterPathWin.end(), '/', '\\');

    // Filter定義用のItemGroupを探す
    XMLElement* filterItemGroup = nullptr;
    for (XMLElement* ig = root->FirstChildElement("ItemGroup"); ig; ig = ig->NextSiblingElement("ItemGroup"))
    {
        if (ig->FirstChildElement("Filter"))
        {
            filterItemGroup = ig;
            break;
        }
    }
    if (!filterItemGroup)
    {
        filterItemGroup = doc.NewElement("ItemGroup");
        root->InsertEndChild(filterItemGroup);
    }

    // パスを分解して親から順に存在確認・作成
    std::string currentPath;
    std::stringstream ss(filterPathWin);
    std::string segment;

    while (std::getline(ss, segment, '\\'))
    {
        if (!currentPath.empty()) currentPath += "\\";
        currentPath += segment;

        bool exists = false;
        for (XMLElement* f = filterItemGroup->FirstChildElement("Filter"); f; f = f->NextSiblingElement("Filter"))
        {
            const char* inc = f->Attribute("Include");
            if (inc && currentPath == inc)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
        {
            XMLElement* filterElem = doc.NewElement("Filter");
            filterElem->SetAttribute("Include", currentPath.c_str());

            XMLElement* uid = doc.NewElement("UniqueIdentifier");
            // 注意: 本来はUUID生成ライブラリを使うべきですが、元の仕様に従い簡易的な値を入れます
            // 必要に応じて std::hash や Windows API (CoCreateGuid) を使用してください
            uid->SetText(("{" + std::to_string(std::hash<std::string>{}(currentPath)) + "}").c_str());

            filterElem->InsertEndChild(uid);
            filterItemGroup->InsertEndChild(filterElem);
        }
    }
}

bool FlAutomaticFileAddSystem::AddToFilters(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath, const std::string& itemType, const std::string& filterPathStr) noexcept
{
	XMLElement* root = doc.RootElement();
	if (!root) return false;

	// ItemTypeに対応するItemGroupを探す
	XMLElement* itemGroup = nullptr;
	for (XMLElement* ig = root->FirstChildElement("ItemGroup"); ig; ig = ig->NextSiblingElement("ItemGroup"))
	{
		if (ig->FirstChildElement(itemType.c_str()))
		{
			itemGroup = ig;
			break;
		}
	}

	if (!itemGroup)
	{
		itemGroup = doc.NewElement("ItemGroup");
		root->InsertEndChild(itemGroup);
	}

	auto relPath = std::filesystem::relative(filePath, m_projectDir).generic_string();
	std::string filterPathWin = filterPathStr;
	std::replace(filterPathWin.begin(), filterPathWin.end(), '/', '\\');

	// 重複チェック
	for (XMLElement* e = itemGroup->FirstChildElement(itemType.c_str()); e; e = e->NextSiblingElement(itemType.c_str()))
	{
		const char* inc = e->Attribute("Include");
		if (inc && relPath == inc) return true;
	}

	XMLElement* elem = doc.NewElement(itemType.c_str());
	elem->SetAttribute("Include", relPath.c_str());

	XMLElement* filterElem = doc.NewElement("Filter");
	filterElem->SetText(filterPathWin.c_str());
	elem->InsertEndChild(filterElem);

	itemGroup->InsertEndChild(elem);
	return true;
}

bool FlAutomaticFileAddSystem::RemoveFromVcxproj(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath) noexcept
{
    XMLElement* root{ doc.RootElement() };
    if (!root) return false;

    auto relPath{ std::filesystem::relative(filePath, m_projectDir).generic_string() };
    auto removed{ false };

    for (XMLElement* ig = root->FirstChildElement("ItemGroup"); ig; )
    {
        XMLElement* nextIg = ig->NextSiblingElement("ItemGroup");
        bool groupEmpty = true;

        for (XMLElement* elem = ig->FirstChildElement(); elem; )
        {
            XMLElement* nextElem = elem->NextSiblingElement();
            const char* include = elem->Attribute("Include");

            if (include && relPath == include)
            {
                ig->DeleteChild(elem);
                removed = true;
            }
            else
            {
                groupEmpty = false;
            }
            elem = nextElem;
        }

        if (groupEmpty && !ig->FirstChildElement())
        {
            root->DeleteChild(ig);
        }
        ig = nextIg;
    }
    return removed;
}

bool FlAutomaticFileAddSystem::RemoveFromFilters(tinyxml2::XMLDocument& doc, const std::filesystem::path& filePath) noexcept
{
    // ロジックは RemoveFromVcxproj とほぼ同じ（Include属性でマッチングして消すだけ）
    return RemoveFromVcxproj(doc, filePath);
}

std::string FlAutomaticFileAddSystem::DetectItemType(const std::filesystem::path& filePath) noexcept
{
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto it = TypeMap.find(ext);
    if (it != TypeMap.end()) {
        return it->second;
    }

    // 未知の拡張子はとりあえずビルド対象外 ("None") にする
    return "None";
}