#pragma once

using SetImGuiContextFunc = void (*) (ImGuiContext*);
using ComponentCreatorFunc = BaseBasicComponent* (*)(const char*, const uint32_t);
using SystemCreatorFunc = BaseBasicSystem* (*)(const char*);
using ClearComponentFunc = bool (*)(const char*, const uint32_t);
using GetSupportedComponentTypeFunc = void (*) (BaseBasicSystem*, char**);
using GetComponentTypesFunc = void (*) (const char***, int*);
using GetSystemTypesFunc = void (*) (const char***, int*);
using FreeTypesFunc = void (*) (const char**, int);
using FreeStringFunc = void (*) (char*);
using AllClearFunc = void (*) ();

class FlCSDLLController
{
public:
    FlCSDLLController();
    ~FlCSDLLController();

    bool Load();
    void Unload();
    void CheckReload(std::unordered_map<int, std::shared_ptr<Entity>>& entityMap, std::vector<BaseBasicSystem*>& systems, std::list<int32_t>& freeIds);

    void ProcessEntityRequests(BaseBasicSystem* sys);

    BaseBasicComponent* CreateComponent(const std::string& type, const uint32_t id) const;
    BaseBasicSystem* CreateSystem(const std::string& type) const;

    bool ClearComponent(const std::string& type, const int32_t id) const;

    bool SaveEntityComponentsToJson(const std::filesystem::path& path,
        const std::unordered_map<int, std::shared_ptr<Entity>>& entityMap);
    bool LoadEntityComponentsFromJson(const std::filesystem::path& path,
        std::unordered_map<int, std::shared_ptr<Entity>>& entityMap);

    bool CreatePrefab(const std::filesystem::path& path, const std::shared_ptr<Entity>& rootEntity);
    std::shared_ptr<Entity> InstantiatePrefab(const std::filesystem::path& path, std::shared_ptr<Entity> parent = nullptr);

    const std::string GetSupportedComponentType(BaseBasicSystem* system) const;

    const std::vector<std::string>& GetComponentTypes() const;
    const std::vector<std::string>& GetSystemTypes() const;

private:
    void CopyDllFiles() const; // DLL/PDBÉRÉsÅ[ä÷êî

    void PopulateComponentTypes();
    void PopulateSystemTypes();

    bool CheckSupportComponent(BaseBasicComponent* comp);

#if _DEBUG
    const std::string m_sourceDllPath{ "Src/Framework/ComponentSystem/DLL/Debug/FlComponentSystem.dll" };
    const std::string m_hotloadDllPath{ "Src/Framework/ComponentSystem/DLL/Debug/FlComponentSystem_hotload.dll" };
    const std::string m_hotloadPdbPath{ "Src/Framework/ComponentSystem/DLL/Debug/FlComponentSystem_hotload.pdb" };
#else
    const std::string m_sourceDllPath{ "Src/Framework/ComponentSystem/DLL/Release/FlComponentSystem.dll" };
    const std::string m_hotloadDllPath{ "Src/Framework/ComponentSystem/DLL/Release/FlComponentSystem_hotload.dll" };
    const std::string m_hotloadPdbPath{ "Src/Framework/ComponentSystem/DLL/Release/FlComponentSystem_hotload.pdb" };
#endif

    HMODULE m_dllHandle;

    std::vector<std::string> m_componentTypes;
    std::vector<std::string> m_systemTypes;

    SetImGuiContextFunc           m_setImGuiContext;
    ComponentCreatorFunc          m_createComponent;
    SystemCreatorFunc             m_createSystem;
    ClearComponentFunc            m_clearComponent;
    GetSupportedComponentTypeFunc m_getSupportedComponentType;
    GetComponentTypesFunc         m_getComponentTypes;
    GetSystemTypesFunc            m_getSystemTypes;
    FreeTypesFunc                 m_freeTypes;
    FreeStringFunc                m_freeString;
    AllClearFunc                  m_allClear;

    std::unordered_map<int32_t, std::vector<std::pair<std::string, std::vector<char>>>> m_entityComponentData;
    std::unordered_map<int32_t, std::pair<int32_t, std::vector<int32_t>>> m_entityRelations; // parent ID and child IDs
    std::filesystem::file_time_type m_lastWriteTime;

    std::unique_ptr<FlChronus::Ticker> m_upTicker;
};