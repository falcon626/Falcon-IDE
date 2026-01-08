static FlRuntimeAPI* g_runtimeApi { nullptr };

extern "C" __declspec(dllexport) FlResult SetAPI(FlRuntimeAPI* api, void* ctx) noexcept
{
    try {
        g_runtimeApi = api;
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx));

        if (g_runtimeApi)
        {
            ComponentReflection reflection{
                // Create
                []() noexcept -> void* {
                    try {
                        auto p{ new PlayerComponent() };
                        auto start{ [](void* component) noexcept {
                        try {
                                                  if (!component)
                         {
                             g_runtimeApi->ToLogError("Start: Null component(%s) pointer.", "PlayerComponent");
                             return;
                         }
                         auto c{ static_cast<PlayerComponent*>(component) };
                                              
                                              
                        }
                        catch (...) {
                            g_runtimeApi->ToLogError("Start: Throw to start component(%s).", "PlayerComponent");
                            return;
                        } } };
                        start(p);
                        return p;
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Create: Throw to create component(%s).", "PlayerComponent");
                        return nullptr;
                    }
                },
                // Destroy
                [](void* component) noexcept {
                    try {
                        if (!component)
                        {
                            g_runtimeApi->ToLogError("Destroy: Null component(%s) pointer.", "PlayerComponent");
                            return;
                        }
                        auto onDestroy{ [](void* component) noexcept {
                            try {
                                                  if (!component)
                         {
                             g_runtimeApi->ToLogError("OnDestroy: Null component(%s) pointer.", "PlayerComponent");
                             return;
                         }
                         auto c{ static_cast<PlayerComponent*>(component) };
                                              
                                              
                            }
                            catch (...) {
                                g_runtimeApi->ToLogError("OnDestroy: Throw to onDestroy component(%s).", "PlayerComponent");
                                return;
                            } } };
                        onDestroy(component);
                        auto c{ static_cast<PlayerComponent*>(component) };
                        delete c;
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Destroy: Throw to destroy component(%s).", "PlayerComponent");
                        return;
                    }
                },
                // Copy
                [](void* component) noexcept -> void* {
                    try {
                        if (!component)
                        {
                            g_runtimeApi->ToLogError("Copy: Null component(%s) pointer.", "PlayerComponent");
                            return nullptr;
                        }
                        auto c{ static_cast<PlayerComponent*>(component) };
                        return new PlayerComponent(*c);
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Copy: Throw to copy component(%s).", "PlayerComponent");
                        return nullptr;
                    }

                },
                // Serialize
                [](void* component, nlohmann::json& json) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("Serialize: Null component(%s) pointer.", "PlayerComponent");
                             return;
                         }
                         auto c{ static_cast<PlayerComponent*>(component) };
                                                  json["Speed"] = c->m_speed;
                     
                         json["Repop"]["x"] = c->m_repopPos.x;
                         json["Repop"]["y"] = c->m_repopPos.y;
                         json["Repop"]["z"] = c->m_repopPos.z;
                     
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Serialize: Throw to serialize logic(%s).", "Player");
                        return;
                    }
                },
                // Deserialize
                [](void* component, const nlohmann::json& json) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("Deserialize: Null component(%s) pointer.", "PlayerComponent");
                             return;
                         }
                         auto c{ static_cast<PlayerComponent*>(component) };
                                                  FlJsonUtility::GetValue(json, "Speed", &c->m_speed);
                     
                         if (json.contains("Repop"))
                         {
                             json.at("Repop").at("x").get_to(c->m_repopPos.x);
                             json.at("Repop").at("y").get_to(c->m_repopPos.y);
                             json.at("Repop").at("z").get_to(c->m_repopPos.z);
                         }
                     
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Deserialize: Throw to deserialize logic(%s).", "Player");
                        return;
                    }
                },
                // RenderEditor
                [](void* component, entityId id) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("RenderEditor: Null component(%s) pointer.", "PlayerComponent");
                             return;
                         }
                         auto c{ static_cast<PlayerComponent*>(component) };
                                              
                         ImGui::DragFloat("Speed", &c->m_speed);
                         ImGui::DragFloat3("Repop Pos", &c->m_repopPos.x);
                     
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("RenderEditor: Throw to renderEditor logic(%s).", "Player");
                        return;
                    }
                },
                // Update
                [](void* component, entityId id, float deltaTime) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("Update: Null component(%s) pointer.", "PlayerComponent");
                             return;
                         }
                         auto c{ static_cast<PlayerComponent*>(component) };
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
                    catch (...) {
                        g_runtimeApi->ToLogError("Update: Throw to update logic(%s).", "Player");
                        return;
                    }
                }
            };

            g_runtimeApi->RegisterModule("Player", &reflection);
            g_runtimeApi->ToLogInfo("Regist: %s", "Player");
            return { FlResult::Fl_OK, __LINE__ };
        }
        else return { FlResult::Fl_FAIL, __LINE__ };
    }
    catch (...) {
        return { FlResult::Fl_THROW, __LINE__ };
    }
}