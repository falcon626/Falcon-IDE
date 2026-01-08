#pragma once

#ifdef _WIN32
#ifdef FL_COMPONENTSYSTEM_BUILD
#define FL_COMPONENTSYSTEM_API __declspec(dllexport)
#else
#define FL_COMPONENTSYSTEM_API __declspec(dllimport)
#endif
#else
#define FL_COMPONENTSYSTEM_API
#endif

struct EntityRequest 
{
    enum class ActionType { None, AddComponent, RemoveComponent, Create, Destroy, CreateChild, CreatePrefab, LoadScene };
    ActionType action_{ ActionType::None };
    uint32_t entityId_{ Def::UIntZero };
    std::string componentType_;
    std::string prefab_;
    std::string scene_;
    void* data_;
};

class BaseBasicComponent
{
public:
    virtual ~BaseBasicComponent() = default;
    virtual std::string GetType() const PURE;
    virtual void Serialize(nlohmann::json& json) const PURE;
    virtual void Deserialize(nlohmann::json& json) PURE;
    virtual void RenderImGui() PURE;

    virtual void OnAddedToEntity([[maybe_unused]] uint32_t entityId) {}
    virtual void OnRemovedFromEntity([[maybe_unused]] uint32_t entityId) {}

    const auto IsEnable() const noexcept { return m_isEnable; };
protected:
    bool m_isEnable{ true };
};

class BaseModelRenderComponent :public BaseBasicComponent
{
public:
    virtual ~BaseModelRenderComponent() override = default;

    struct ModelRenderData
    {
        bool isLoading_{ false };
        Math::Matrix mWorld_;
        std::string modelPath_;
        ModelData model_;
    };

    const auto& GetModelRenderData() const noexcept { return m_renderData; }
    auto& WorkModelRenderData() noexcept { return m_renderData; }

    const auto& GetModelData() const noexcept { return m_renderData.model_; }
    auto& WorkModelData() noexcept { return m_renderData.model_; }

    auto SetModelData(const ModelData& model) noexcept { m_renderData.model_ = model; }
protected:
    ModelRenderData m_renderData;
};

class BaseCollisionComponent :public BaseBasicComponent
{
public:
    virtual ~BaseCollisionComponent() override = default;
    auto& WorkIsHit() noexcept { return m_isHit; }
protected:
    bool m_isHit{ false };
};

class BaseCameraComponent :public BaseBasicComponent
{
public:
    virtual ~BaseCameraComponent() override = default;

    auto SetViewMatrix(const Math::Matrix& viewMat) noexcept { m_viewMat = viewMat; }
    auto SetProjMatrix(const Math::Matrix& projMat) noexcept { m_projMat = projMat; }

    const auto& GetViewMatrix() const noexcept { return m_viewMat; }
    const auto& GetProjMatrix() const noexcept { return m_projMat; }

protected:
    Math::Matrix m_viewMat;
    Math::Matrix m_projMat;
};

class BaseBasicSystem 
{
public:
    virtual ~BaseBasicSystem() = default;
    virtual void Update(BaseBasicComponent* component, const uint32_t id, float deltaTime) PURE;
    virtual std::string GetType() const PURE;
    virtual std::string GetSupportedComponentType() const PURE;
    virtual void AddChild([[maybe_unused]] BaseBasicComponent* component, const [[maybe_unused]] uint32_t id) {};

    void RequestEntityAction(EntityRequest::ActionType actionType, const uint32_t id, const std::string& componentType, const std::string& prefab = std::string(""), const std::string& scene = std::string(""), void* data = nullptr) noexcept {
        m_entityRequests.push_back({ actionType, id, componentType, prefab, scene, data });
    }

    auto& WorkEntityRequests() noexcept { return m_entityRequests; }
private:
    std::vector<EntityRequest> m_entityRequests;
};

class BaseCollisionSystem : public BaseBasicSystem 
{
public:
    virtual ~BaseCollisionSystem() override = default;
    virtual void Update(BaseBasicComponent*, const uint32_t, float) override {};

    virtual void UpdateCollision(BaseBasicComponent* component, const uint32_t id, float deltaTime) PURE;
};

struct Entity
{
    int32_t id{ Def::IntZero };
    std::string name;
    std::vector<BaseBasicComponent*> components;
    std::weak_ptr<Entity> parent;
    std::vector<std::weak_ptr<Entity>> children;

    void Reset() noexcept
    {
        id = Def::UIntZero;

        for (auto& comp : components)
            comp = nullptr;
        components.clear();

        parent.reset();
        children.clear();
    }

    Entity() : name("Entity " + std::to_string(id)) {}
    ~Entity() { Reset(); }
};

struct FlResult
{
    enum : uint32_t
    {
        RS_OK,
        RS_FAIL
    }code_;
    uint32_t line_{ Def::UIntZero };
};

#ifdef __cplusplus
extern "C"
#endif
{
    // ImGuiコンテキスト
    FL_COMPONENTSYSTEM_API void SetImGuiContext(ImGuiContext* ctx);
    // コンポーネント管理
    FL_COMPONENTSYSTEM_API BaseBasicComponent* CreateComponent(const char* type, const uint32_t id);
    // システム管理
    FL_COMPONENTSYSTEM_API BaseBasicSystem* CreateSystem(const char* type);
    FL_COMPONENTSYSTEM_API void GetSupportedComponentType(BaseBasicSystem* system, char** out_type);
    // 型リスト取得
    FL_COMPONENTSYSTEM_API void GetComponentTypes(const char*** out_types, int* out_count);
    FL_COMPONENTSYSTEM_API void GetSystemTypes(const char*** out_types, int* out_count);
    // ポインタ解放
    FL_COMPONENTSYSTEM_API void FreeTypes(const char** types, int count);
    FL_COMPONENTSYSTEM_API void FreeString(char* str);
    FL_COMPONENTSYSTEM_API void AllClear();
}