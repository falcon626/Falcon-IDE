#include "FlCSDLLController.h"

FlCSDLLController::FlCSDLLController() : m_dllHandle{ nullptr }, m_setImGuiContext{ nullptr }, m_createComponent{ nullptr },
m_createSystem{ nullptr }, m_clearComponent{ nullptr }, m_getSupportedComponentType{ nullptr }, m_getComponentTypes{ nullptr },
m_getSystemTypes{ nullptr }, m_freeTypes{ nullptr }, m_freeString{ nullptr }, m_allClear{ nullptr }, m_upTicker{ std::make_unique<FlChronus::Ticker>(std::chrono::milliseconds(500)) }
{
    try {
        if (std::filesystem::exists(m_hotloadPdbPath)) {
            m_lastWriteTime = std::filesystem::last_write_time(m_hotloadPdbPath);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Filesystem error: %s", e.what());
    }
}

FlCSDLLController::~FlCSDLLController()
{
    Unload();
}

bool FlCSDLLController::Load()
{
    Unload();

    // DLL/PDBをコピーしてロックを回避
    CopyDllFiles();

    // コピーしたホットリロード用のDLLをロードする
    m_dllHandle = LoadLibrary(ansi_to_wide(m_hotloadDllPath).c_str());
    if (!m_dllHandle)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to load DLL: %s, ErrorCode: %ld", m_hotloadDllPath.c_str(), GetLastError());
        return false;
    }

    m_setImGuiContext = reinterpret_cast<SetImGuiContextFunc>
        (GetProcAddress(m_dllHandle, "SetImGuiContext"));
    m_createComponent = reinterpret_cast<ComponentCreatorFunc>
        (GetProcAddress(m_dllHandle, "CreateComponent"));
    m_createSystem = reinterpret_cast<SystemCreatorFunc>
        (GetProcAddress(m_dllHandle, "CreateSystem"));
    m_clearComponent = reinterpret_cast<ClearComponentFunc>
        (GetProcAddress(m_dllHandle, "ClearComponent"));
    m_getSupportedComponentType = reinterpret_cast<GetSupportedComponentTypeFunc>
        (GetProcAddress(m_dllHandle, "GetSupportedComponentType"));
    m_getComponentTypes = reinterpret_cast<GetComponentTypesFunc>
        (GetProcAddress(m_dllHandle, "GetComponentTypes"));
    m_getSystemTypes = reinterpret_cast<GetSystemTypesFunc>
        (GetProcAddress(m_dllHandle, "GetSystemTypes"));
    m_freeTypes = reinterpret_cast<FreeTypesFunc>
        (GetProcAddress(m_dllHandle, "FreeTypes"));
    m_freeString = reinterpret_cast<FreeStringFunc>
        (GetProcAddress(m_dllHandle, "FreeString"));
    m_allClear = reinterpret_cast<AllClearFunc>
        (GetProcAddress(m_dllHandle, "AllClear"));

    if (!m_setImGuiContext || !m_createComponent || !m_createSystem || !m_clearComponent || !m_getSupportedComponentType ||
        !m_getComponentTypes || !m_getSystemTypes || !m_freeTypes || !m_freeString || !m_allClear)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to get function pointers");
        FreeLibrary(m_dllHandle);
        m_dllHandle = nullptr;
        return false;
    }


    m_setImGuiContext(FlEditorAdministrator::Instance().GetContext());

    PopulateComponentTypes();
    PopulateSystemTypes();

    return true;
}

void FlCSDLLController::Unload()
{
    if (m_allClear) m_allClear();

    if (m_dllHandle)
    {
        FreeLibrary(m_dllHandle);
        m_dllHandle = nullptr;
    }

    m_setImGuiContext           = nullptr;
    m_createComponent           = nullptr;
    m_createSystem              = nullptr;
    m_clearComponent            = nullptr;
    m_getSupportedComponentType = nullptr;
    m_getComponentTypes         = nullptr;
    m_getSystemTypes            = nullptr;
    m_freeTypes                 = nullptr;
    m_freeString                = nullptr;
    m_allClear                  = nullptr;

    m_systemTypes.clear();
    m_componentTypes.clear();
}

void FlCSDLLController::CheckReload(std::unordered_map<int, std::shared_ptr<Entity>>& entityMap, std::vector<BaseBasicSystem*>& systems, std::list<int32_t>& freeIds)
{
    // <QuickReturn:まだ間隔が経過していなければ処理しない>
    if (!m_upTicker->tick()) return;

    try {
        // 監視対象のPDBファイルが存在しない場合は何もしない
        if (!std::filesystem::exists(m_hotloadPdbPath)) return;

        // 監視対象をホットリロード用のPDBファイルに変更
        auto currentWriteTime{ std::filesystem::last_write_time(m_hotloadPdbPath) };

        if (m_lastWriteTime != currentWriteTime)
        {
            auto scoped{ FlChronus::Scoped{[](FlChronus::Dur d) {
                auto ms{ std::chrono::duration_cast<FlChronus::ms>(d) };
                FlEditorAdministrator::Instance().GetLogger()->AddLog("DLL reloaded time: %dms", ms.count());
                } } };

            FlEditorAdministrator::Instance().GetLogger()->AddLog("DLL modified, reloading...");

            SaveEntityComponentsToJson("Assets/Scene/temp.scene", entityMap);

            entityMap.clear();

            for (auto& sys : systems)
                sys = nullptr;
            systems.clear();

            Unload();

            FlChronus::sleep_for(FlChronus::ms(200));

            CopyDllFiles();

            if (Load())
            {
                LoadEntityComponentsFromJson("Assets/Scene/temp.scene", entityMap);

                for (const auto& type : GetSystemTypes()) {
                    auto sys{ CreateSystem(type) };
                    if (sys) systems.emplace_back(sys);
                }

                m_lastWriteTime = currentWriteTime;
                FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Successfully reloaded DLL");

                return;
            }
            else 
            {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to reload DLL");
                return;
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Filesystem error during check reload: %s", e.what());
        // タイムスタンプをリセットして、次回のチェックを促す
        m_lastWriteTime = {};
        return;
    }
    return;
}

void FlCSDLLController::ProcessEntityRequests(BaseBasicSystem* sys)
{
    decltype(auto) requests{ sys->WorkEntityRequests() };
    if (requests.empty()) return;

    for (const auto& request : requests) {

        switch (request.action_) 
        {
        case EntityRequest::ActionType::AddComponent:
            FlScene::Instance().AttachComponent(request.entityId_, request.componentType_);
            break;
        case EntityRequest::ActionType::RemoveComponent:
            FlScene::Instance().DetachComponent(request.entityId_, request.componentType_);
            break;
        case EntityRequest::ActionType::Create:
            FlScene::Instance().CreateEntity();
            break;
        case EntityRequest::ActionType::Destroy:
            FlScene::Instance().DeleteEntityLater(request.entityId_);
            break;
        case EntityRequest::ActionType::CreateChild:
            FlScene::Instance().CreateEntity(FlScene::Instance().GetEntityById(request.entityId_).lock());
            break;
        case EntityRequest::ActionType::CreatePrefab:
            InstantiatePrefab(request.prefab_, FlScene::Instance().GetRootEntity());
            break;
        case EntityRequest::ActionType::LoadScene:
            FlScene::Instance().LodeScene(request.scene_);
            break;
        }
    }

    // リクエストをクリアして再利用可能に
    requests.clear();
}

BaseBasicComponent* FlCSDLLController::CreateComponent(const std::string& type, const uint32_t id) const
{
    return m_createComponent ? m_createComponent(type.c_str(), id) : nullptr;
}

BaseBasicSystem* FlCSDLLController::CreateSystem(const std::string& type) const
{
    return m_createSystem ? m_createSystem(type.c_str()) : nullptr;
}

bool FlCSDLLController::ClearComponent(const std::string& type, const int32_t id) const
{
    return m_clearComponent(type.c_str(), id);
}

const std::string FlCSDLLController::GetSupportedComponentType(BaseBasicSystem* system) const
{
    if (m_getSupportedComponentType && system)
    {
        auto str{ static_cast<char*>(NULL) };
        m_getSupportedComponentType(system, &str);

        auto result{ std::string{} };
        if (str) {
            result = str; // ここでコピー
            m_freeString(str); // コピー後に解放
        }
        return result;
    }
    return std::string{};
}

const std::vector<std::string>& FlCSDLLController::GetComponentTypes() const
{
    return m_componentTypes;
}

const std::vector<std::string>& FlCSDLLController::GetSystemTypes() const
{
    return m_systemTypes;
}

void FlCSDLLController::CopyDllFiles() const
{
    try {
        std::filesystem::copy(m_sourceDllPath, m_hotloadDllPath, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy(m_sourceDllPath.substr(0, m_sourceDllPath.find_last_of('.')) + ".pdb", m_hotloadPdbPath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& e) {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to copy DLL files: %s", e.what());
    }
}

bool FlCSDLLController::SaveEntityComponentsToJson(const std::filesystem::path& path,
    const std::unordered_map<int, std::shared_ptr<Entity>>& entityMap)
{
    using json = nlohmann::json;
    auto root{ json::object() };
    auto entities{ json::object() };

    for (const auto& [id, entityPtr] : entityMap) {
        if (!entityPtr) continue;

        auto entJson{ json::object() };

        entJson["name"] = entityPtr->name;

        // コンポーネント
        auto comps{ json::array() };
        for (auto* comp : entityPtr->components) {
            json j;
            j["type"] = comp->GetType();
            comp->Serialize(j);
            comps.push_back(j);
        }
        entJson["components"] = comps;

        // 親子関係
        entJson["parentId"] = entityPtr->parent.lock() ? entityPtr->parent.lock()->id : -1;
        auto childIds{ json::array() };
        for (auto& w : entityPtr->children) {
            if (auto c = w.lock()) 
                childIds.push_back(c->id);
        }
        entJson["childIds"] = childIds;

        entities[std::to_string(id)] = entJson;
    }

    root["entities"] = entities;

    return FlJsonUtility::Serialize(root, path);
}


bool FlCSDLLController::LoadEntityComponentsFromJson(const std::filesystem::path& path,
    std::unordered_map<int, std::shared_ptr<Entity>>& entityMap)
{
    using json = nlohmann::json;
    json root;
    if (!FlJsonUtility::Deserialize(root, path)) return false;
    if (!root.contains("entities") || !root["entities"].is_object()) return false;

    entityMap.clear();
    auto entities{ root["entities"] };
    // エンティティ作成およびコンポーネント復元
    for (auto it{ entities.begin() }; it != entities.end(); ++it) {
        auto id{ std::stoi(it.key()) };
        auto entJson{ it.value() };

        auto newEnt{ std::make_shared<Entity>() };
        newEnt->id = id;

        newEnt->name = entJson.value("name", "Entity_" + std::to_string(id));

        if (entJson.contains("components") && entJson["components"].is_array())
        {
            for (auto& compJson : entJson["components"]) {
                auto type{ compJson.value("type", "") };
                auto comp{ CreateComponent(type, id) };
                if (comp) 
                {
                    comp->Deserialize(compJson);
                    newEnt->components.emplace_back(comp);
                }
            }
        }

        entityMap[id] = newEnt;
    }

    // 親子関係を復元
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        int id = std::stoi(it.key());
        auto entJson = it.value();
        auto ent = entityMap[id];
        if (!ent) continue;

        int parentId = entJson.value("parentId", -1);
        if (parentId != -Def::IntOne && entityMap.count(parentId)) {
            ent->parent = entityMap[parentId];
            entityMap[parentId]->children.emplace_back(ent);

            for (auto& sys : FlScene::Instance().GetSystems())
            {
                auto supportedType{ GetSupportedComponentType(sys) };
                for (auto& component : entityMap[parentId]->components)
                {
                    for (auto& compChi : ent->components)
                    {
                        if ((component->GetType() == supportedType) && (compChi->GetType() == supportedType))
                            sys->AddChild(component, ent->id);
                    }
                }
            }
        }
    }

    return true;
}

bool FlCSDLLController::CreatePrefab(const std::filesystem::path& path, const std::shared_ptr<Entity>& rootEntity)
{
    using json = nlohmann::json;
    auto root{ json::object() };
    auto entities{ json::object() };

    // シーンIDからプレハブ内ローカルIDへの変換マップ
    std::unordered_map<int, int> sceneIdToPrefabId;
    auto nextPrefabId{ Def::IntZero };

    // 再帰的にエンティティを走査してシリアライズするラムダ関数
    std::function<void(const std::shared_ptr<Entity>&)> serializeRecursive{
        [&](const std::shared_ptr<Entity>& currentEntity)
        {
            if (!currentEntity) return;

            // --- プレハブ内でのローカルIDを割り当て ---
            auto prefabId{ nextPrefabId++ };
            sceneIdToPrefabId[currentEntity->id] = prefabId;

            auto entJson{ json::object() };
            entJson["name"] = currentEntity->name;

            // --- コンポーネントのシリアライズ ---
            auto comps{ json::array() };
            for (auto* comp : currentEntity->components) {
                json j;
                j["type"] = comp->GetType();
                comp->Serialize(j);
                comps.push_back(j);
            }
            entJson["components"] = comps;

            // --- 親子関係のシリアライズ（プレハブ内ローカルIDを使用） ---
            auto parentPrefabId{ -Def::IntOne };
            if (auto parent = currentEntity->parent.lock()) {
                // 親がプレハブ階層内に存在する場合のみIDを記録
                if (sceneIdToPrefabId.count(parent->id)) {
                    parentPrefabId = sceneIdToPrefabId[parent->id];
                }
            }
            entJson["parentId"] = parentPrefabId;

            // このエンティティをJSONオブジェクトに追加
            entities[std::to_string(prefabId)] = entJson;

            // 子エンティティを再帰的に処理
            for (const auto& child_weak : currentEntity->children) {
                if (auto child_shared = child_weak.lock())
                    serializeRecursive(child_shared);
            }
        }
    };

    // 指定されたルートエンティティからシリアライズを開始
    serializeRecursive(rootEntity);

    root["entities"] = entities;

    return FlJsonUtility::Serialize(root, path);
}

std::shared_ptr<Entity> FlCSDLLController::InstantiatePrefab(
    const std::filesystem::path& path,
    std::shared_ptr<Entity> parent)
{
    using json = nlohmann::json;
    json root;
    if (!FlJsonUtility::Deserialize(root, path)) {
        return nullptr;
    }
    if (!root.contains("entities") || !root["entities"].is_object()) {
        return nullptr;
    }

    json entities = root["entities"];
    std::unordered_map<int, std::shared_ptr<Entity>> prefabIdToNewEntity;

    std::shared_ptr<Entity> prefabRootEntity = nullptr;
    auto& scene = FlScene::Instance();

    // --- パス1: 新しい Entity を作成 ---
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        int prefabId = std::stoi(it.key());
        auto entJson = it.value();

        // FlScene::CreateEntity で新しい Scene 内 ID を発行
        auto newEnt = scene.CreateEntity(entJson);

        // --- 名前を上書き ---
        newEnt->name = entJson.value("name", "Entity_" + std::to_string(newEnt->id));

        // --- コンポーネントの復元 ---
        if (entJson.contains("components") && entJson["components"].is_array()) {
            for (auto& compJson : entJson["components"]) {
                std::string type = compJson.value("type", "");
                auto comp = CreateComponent(type, newEnt->id);
                if (comp) {
                    comp->Deserialize(compJson);
                    newEnt->components.emplace_back(comp);
                }
            }
        }

        prefabIdToNewEntity[prefabId] = newEnt;
    }

    // --- パス2: 親子関係を復元 ---
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        int prefabId = std::stoi(it.key());
        auto entJson = it.value();

        auto currentEnt = prefabIdToNewEntity[prefabId];
        int parentPrefabId = entJson.value("parentId", -1);

        if (parentPrefabId != -1) {
            auto newParentEnt = prefabIdToNewEntity[parentPrefabId];
            currentEnt->parent = newParentEnt;

            for (auto& sys : FlScene::Instance().GetSystems())
            {
                auto supportedType{ GetSupportedComponentType(sys) };
                for (auto& component : newParentEnt->components)
                {
                    for (auto& compChi : currentEnt->components)
                    {
                        if ((component->GetType() == supportedType) && (compChi->GetType() == supportedType))
                        {
                            sys->AddChild(component, currentEnt->id);
                        }
                    }
                }
            }

            newParentEnt->children.emplace_back(currentEnt);
        }
        else {
            // Prefab 内の Root
            prefabRootEntity = currentEnt;
        }
    }

    // --- 最終処理: シーンにアタッチ ---
    if (prefabRootEntity && parent) {
        parent->children.emplace_back(prefabRootEntity);
        prefabRootEntity->parent = parent;
    }

    return prefabRootEntity;
}


void FlCSDLLController::PopulateComponentTypes()
{
    const char** componentTypesC{ nullptr };
    auto count{ Def::IntZero };

    m_getComponentTypes(&componentTypesC, &count);

    if (count > 0 && componentTypesC != nullptr)
    {
        m_componentTypes.clear();
        m_componentTypes.reserve(count);

        for (auto i{ 0 }; i < count; ++i) {
            m_componentTypes.emplace_back(componentTypesC[i]);
        }

        m_freeTypes(componentTypesC, count);
    }
    else
    {
        m_componentTypes.clear();
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Warning: Failed to get component types or no types available.");
    }
}

void FlCSDLLController::PopulateSystemTypes()
{
    const char** systemTypesC{ nullptr };
    auto count{ Def::IntZero };

    m_getSystemTypes(&systemTypesC, &count);

    if (count > 0 && systemTypesC != nullptr)
    {
        m_systemTypes.clear();
        m_systemTypes.reserve(count);

        for (auto i{ 0 }; i < count; ++i) {
            m_systemTypes.emplace_back(systemTypesC[i]);
        }

        m_freeTypes(systemTypesC, count);
    }
    else
    {
        m_systemTypes.clear();
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Warning: Failed to get system types or no types available.");
    }
}

bool FlCSDLLController::CheckSupportComponent(BaseBasicComponent* comp)
{
    for (auto& supComp : m_componentTypes)
    {
        if (comp->GetType() == supComp) return true;
        else continue;
    }
    return false;
}
