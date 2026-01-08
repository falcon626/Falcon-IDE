static FlRuntimeAPI* g_runtimeApi { nullptr };

typedef struct PlayerComponent
{
    bool m_isEnable{ true };
    float m_speed{ 0.01f };
    Math::Vector3 m_repopPos;
}PlayerComponent;

/// <summary>
/// コンポーネントCreate時の処理
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <returns>無し</returns>
void Start(void* component) noexcept
{
    // <Never Change>
    if (!component)
    {
        g_runtimeApi->ToLogError("Start: Null component(%s) pointer.", "PlayerComponent");
        return;
    }
    auto c{ static_cast<PlayerComponent*>(component) };
    // </Never Change>

    // TODO: スタート処理実装
}

/// <summary>
/// コンポーネントDestroy時の処理
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <returns>無し</returns>
void OnDestroy(void* component) noexcept
{
    // <Never Change>
    if (!component)
    {
        g_runtimeApi->ToLogError("OnDestroy: Null component(%s) pointer.", "PlayerComponent");
        return;
    }
    auto c{ static_cast<PlayerComponent*>(component) };
    // </Never Change>

    // TODO: オンデストロイ処理実装
}

/// <summary>
/// Componentデータの保存処理
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="json">nlohmann::jsonの保存用変数</param>
/// <returns>無し</returns>
void Serialize(void* component, nlohmann::json& json) noexcept
{
    // <Never Change>
    if(!component)
    {
        g_runtimeApi->ToLogError("Serialize: Null component(%s) pointer.", "PlayerComponent");
        return;
    }
    auto c{ static_cast<PlayerComponent*>(component) };
    // </Never Change>
    json["Speed"] = c->m_speed;

    json["Repop"]["x"] = c->m_repopPos.x;
    json["Repop"]["y"] = c->m_repopPos.y;
    json["Repop"]["z"] = c->m_repopPos.z;
}

/// <summary>
/// Componentデータの読込処理
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="json">nlohmann::jsonの読込用定数</param>
/// <returns>無し</returns>
void Deserialize(void* component, const nlohmann::json& json) noexcept
{
    // <Never Change>
    if(!component)
    {
        g_runtimeApi->ToLogError("Deserialize: Null component(%s) pointer.", "PlayerComponent");
        return;
    }
    auto c{ static_cast<PlayerComponent*>(component) };
    // </Never Change>
    FlJsonUtility::GetValue(json, "Speed", &c->m_speed);

    if (json.contains("Repop"))
    {
        json.at("Repop").at("x").get_to(c->m_repopPos.x);
        json.at("Repop").at("y").get_to(c->m_repopPos.y);
        json.at("Repop").at("z").get_to(c->m_repopPos.z);
    }
}

/// <summary>
/// エディタ処理
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="id">uint32_t型の現在処理中のEntity</param>
/// <returns>無し</returns>
void RenderEditor(void* component, entityId id) noexcept
{
    // <Never Change>
    if(!component)
    {
        g_runtimeApi->ToLogError("RenderEditor: Null component(%s) pointer.", "PlayerComponent");
        return;
    }
    auto c{ static_cast<PlayerComponent*>(component) };
    // </Never Change>

    ImGui::DragFloat("Speed", &c->m_speed);
    ImGui::DragFloat3("Repop Pos", &c->m_repopPos.x);
}

/// <summary>
/// Componentデータの更新ロジック
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="id">uint32_t型の現在処理中のEntity</param>
/// <param name="deltaTime">デルタタイム</param>
/// <returns>無し</returns>
void Update(void* component, entityId id, float deltaTime) noexcept
{
    // <Never Change>
    if(!component)
    {
        g_runtimeApi->ToLogError("Update: Null component(%s) pointer.", "PlayerComponent");
        return;
    }
    auto c{ static_cast<PlayerComponent*>(component) };
    // </Never Change>
    auto tc{ static_cast<TransformComponent*>(g_runtimeApi->GetComponent("Transform",id)) };
    if (!tc) return;

    auto pos{ tc->m_transform->GetLocalPosition() };

    if (c->m_isEnable)
    {
        if (GetAsyncKeyState(VK_RIGHT))pos.z -= c->m_speed * deltaTime;
        if (GetAsyncKeyState(VK_LEFT)) pos.z += c->m_speed * deltaTime;
    }
    else
    {
        pos = c->m_repopPos;
        c->m_isEnable = true;
    }
    tc->m_transform->SetLocalPosition(pos);

    auto cc{ static_cast<CollisionComponent*>(g_runtimeApi->GetComponent("Collision",id)) };
    if (!cc) return;

    if (cc->m_isHit) c->m_isEnable = false;
}