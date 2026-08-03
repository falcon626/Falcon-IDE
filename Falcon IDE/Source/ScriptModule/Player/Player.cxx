static FlRuntimeAPI* g_runtimeApi { nullptr };

typedef struct PlayerComponent
{
    bool m_isEnable{ true };
    float m_speed{ 0.01f };
    Math::Vector3 m_repopPos;
}PlayerComponent;

/// <summary>
/// 繧ｳ繝ｳ繝昴・繝阪Φ繝・reate譎ゅ・蜃ｦ逅・
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <returns>辟｡縺・/returns>
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

    // TODO: 繧ｹ繧ｿ繝ｼ繝亥・逅・ｮ溯｣・
}

/// <summary>
/// 繧ｳ繝ｳ繝昴・繝阪Φ繝・estroy譎ゅ・蜃ｦ逅・
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <returns>辟｡縺・/returns>
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

    // TODO: 繧ｪ繝ｳ繝・せ繝医Ο繧､蜃ｦ逅・ｮ溯｣・
}

/// <summary>
/// Component繝・・繧ｿ縺ｮ菫晏ｭ伜・逅・
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="json">nlohmann::json縺ｮ菫晏ｭ倡畑螟画焚</param>
/// <returns>辟｡縺・/returns>
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
/// Component繝・・繧ｿ縺ｮ隱ｭ霎ｼ蜃ｦ逅・
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="json">nlohmann::json縺ｮ隱ｭ霎ｼ逕ｨ螳壽焚</param>
/// <returns>辟｡縺・/returns>
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
/// 繧ｨ繝・ぅ繧ｿ蜃ｦ逅・
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="id">uint32_t蝙九・迴ｾ蝨ｨ蜃ｦ逅・ｸｭ縺ｮEntity</param>
/// <returns>辟｡縺・/returns>
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
/// Component繝・・繧ｿ縺ｮ譖ｴ譁ｰ繝ｭ繧ｸ繝・け
/// </summary>
/// <param name="component">ComponentInstancePointer</param>
/// <param name="id">uint32_t蝙九・迴ｾ蝨ｨ蜃ｦ逅・ｸｭ縺ｮEntity</param>
/// <param name="deltaTime">繝・Ν繧ｿ繧ｿ繧､繝</param>
/// <returns>辟｡縺・/returns>
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