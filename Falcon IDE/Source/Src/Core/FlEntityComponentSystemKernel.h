#pragma once
#include "../Module/FlRunTimeAndDLLsCommon.h++"

class FlEntityComponentSystemKernel
{
public:

    struct ComponentStorage {
        std::unordered_map<uint32_t, void*> components;
        ComponentReflection reflection;
    };

    void initialize();

    /**
     * @brief 新しいIDを発行するか、再利用可能なIDを返す (自動発行)
     * @return 一意のEntity ID
     */
    const entityId CreateEntity(); // FreeList

    /**
     * @brief 指定されたIDを使用してEntityを登録する (指定発行)
     * @param specifiedId 登録したい特定のID
     * @return 登録に成功したか (衝突した場合は false)
     */
    const bool CreateEntity(entityId specifiedId);


    /**
     * @brief 使用済みのIDを解放し、再利用可能にする
     * @param id 解放するEntity ID
     * @return 解放に成功したか (IDが存在しなかった場合は false)
     */
    const bool ReleaseId(entityId id);

    /**
     * @brief 指定されたIDが現在使用中かを確認する
     * @param id 確認するID
     * @return 使用中であれば true
     */
    const bool IsActive(entityId id) const { return m_activeIds.count(id) > Def::UIntZero; }

    /**
     * @brief 現在アクティブなIDの数を返す
     */
    const size_t GetActiveIdCount() const { return m_activeIds.size(); }

	void DestroyEntity(entityId id);

    void AllDestroyEntities();

    void RegisterModule(const std::string& typeName, ComponentReflection refl, const priority prio = Def::BitMaskPos4);

    // コンポーネント追加
    void* AddComponent(const std::string& name, entityId entity);

    // 削除
    void RemoveComponent(const std::string& name, entityId entity);

    // 参照
    void* GetComponent(const std::string& name, entityId entity);

    bool HasComponent(const std::string& name, entityId entity) const;

    // Update（デッドロック回避）
    void UpdateAll(float dt);

    nlohmann::json SerializeEntity(entityId id);

    void DeserializeEntity(entityId id, const nlohmann::json& src);

    nlohmann::json SerializeScene();

    void DeserializeScene(const nlohmann::json& src);

    void DeserializeScene(const nlohmann::json& src, std::unordered_map<entityId, entityId>* outRemap);

    void ToLogInfo(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(m_mu);
        FlEditorAdministrator::Instance().GetLogger()->AddLog(str);
    }

    void ToLogError(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(m_mu);
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(str);
    }

    void ClearComponent(const std::string_view name);

    void RemoveAllComponentsByModule(HMODULE moduleHandle);


    static auto& Instance() noexcept
    {
        static auto instance{ FlEntityComponentSystemKernel{} };
        return instance;
    }

    // エンティティ一覧（vector にして返す）
    std::vector<entityId> GetAllEntityIds() const;

    // 登録されているコンポーネント型一覧（storages_ のキー）
    std::vector<std::string> GetRegisteredComponentTypes() const;

    // 指定エンティティが持つコンポーネント型一覧（serialize を簡易利用）
    std::vector<std::string> GetEntityComponentTypes(entityId id) const;

    // 戻り値: 成功したら true（reflection が存在して RenderEditor を呼んだ）
    bool RenderComponentEditor(const std::string& typeName, entityId id) const;

private:
    FlEntityComponentSystemKernel() = default;
    ~FlEntityComponentSystemKernel() = default;

    enum { Prio, Comp_Name, Comp_Stor };

    auto FindStorageIterator(const std::string_view name)
    {
        return std::find_if(m_storages.begin(), m_storages.end(),
            [&](const auto& t) { return std::get<std::string>(t) == name; });
    }

    auto FindStorageIterator(const std::string_view name) const
    {
        return std::find_if(m_storages.begin(), m_storages.end(),
            [&](const auto& t) { return std::get<std::string>(t) == name; });
    }

    mutable std::mutex m_mu;
    mutable std::mutex m_moduleCallsMu;

    std::unordered_map<HMODULE, std::atomic<int>> m_moduleActiveCalls;
    std::condition_variable_any m_moduleCv;

    std::vector<std::tuple<priority, std::string, ComponentStorage>> m_storages;

    // 現在使用中のIDのセット (衝突回避と存在確認用)
    std::unordered_set<entityId> m_activeIds;

    // 解放され、再利用待ちのIDのセット (指定ID発行時の検索/削除を高速化)
    std::unordered_set<entityId> m_freeIds;

    // 次に発行する「新しい」IDのカウンター
    entityId m_nextId{ Def::UIntZero };
};