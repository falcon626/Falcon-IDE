static FlRuntimeAPI* g_runtimeApi { nullptr };
static HWND g_hWnd{};

typedef struct ReincarnationComponent
{
    Math::Vector3 m_move{ Def::Vec3 };
    Math::Vector3 m_start{ Def::Vec3 };
    float m_limit{ Def::FloatZero };
}ReincarnationComponent;

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
        g_runtimeApi->ToLogError("Start: Null component(%s) pointer.", "ReincarnationComponent");
        return;
    }
    auto c{ static_cast<ReincarnationComponent*>(component) };
    // </Never Change>
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
        g_runtimeApi->ToLogError("OnDestroy: Null component(%s) pointer.", "ReincarnationComponent");
        return;
    }
    auto c{ static_cast<ReincarnationComponent*>(component) };
    // </Never Change>
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
        g_runtimeApi->ToLogError("Serialize: Null component(%s) pointer.", "ReincarnationComponent");
        return;
    }
    auto c{ static_cast<ReincarnationComponent*>(component) };
    // </Never Change>

    json["Limit"] = c->m_limit;

    json["Start"]["x"] = c->m_start.x;
    json["Start"]["y"] = c->m_start.y;
    json["Start"]["z"] = c->m_start.z;

    json["Move"]["x"] = c->m_move.x;
    json["Move"]["y"] = c->m_move.y;
    json["Move"]["z"] = c->m_move.z;
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
    if (!component)
    {
        g_runtimeApi->ToLogError("Deserialize: Null component(%s) pointer.", "ReincarnationComponent");
        return;
    }
    auto c{ static_cast<ReincarnationComponent*>(component) };
    // </Never Change>

    FlJsonUtility::GetValue(json, "Limit", &c->m_limit);

    if (json.contains("Start"))
    {
        json.at("Start").at("x").get_to(c->m_start.x);
        json.at("Start").at("y").get_to(c->m_start.y);
        json.at("Start").at("z").get_to(c->m_start.z);
    }
    if (json.contains("Move"))
    {
        json.at("Move").at("x").get_to(c->m_move.x);
        json.at("Move").at("y").get_to(c->m_move.y);
        json.at("Move").at("z").get_to(c->m_move.z);
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
        g_runtimeApi->ToLogError("RenderEditor: Null component(%s) pointer.", "ReincarnationComponent");
        return;
    }
    auto c{ static_cast<ReincarnationComponent*>(component) };
    // </Never Change>

    ImGui::DragFloat("Limit", &c->m_limit);
    ImGui::DragFloat3("Start Pos", &c->m_start.x);
    ImGui::DragFloat3("MoveSpeed", &c->m_move.x);
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
        g_runtimeApi->ToLogError("Update: Null component(%s) pointer.", "ReincarnationComponent");
        return;
    }
    auto c{ static_cast<ReincarnationComponent*>(component) };
    // </Never Change>

    auto tc{ static_cast<TransformComponent*>(g_runtimeApi->GetComponent("Transform",id)) };
    if (!tc) return;

    if (std::abs(tc->m_transform->GetLocalPosition().x - c->m_start.x) > c->m_limit)
        tc->m_transform->SetLocalPosition(c->m_start);

    auto pos{ tc->m_transform->GetLocalPosition() };
    
    pos += c->m_move * deltaTime;
    tc->m_transform->SetLocalPosition(pos);

}