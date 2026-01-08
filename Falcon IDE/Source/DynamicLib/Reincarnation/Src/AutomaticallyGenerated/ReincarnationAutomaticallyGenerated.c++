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
                        auto p{ new ReincarnationComponent() };
                        auto start{ [](void* component) noexcept {
                        try {
                                                  if (!component)
                         {
                             g_runtimeApi->ToLogError("Start: Null component(%s) pointer.", "ReincarnationComponent");
                             return;
                         }
                         auto c{ static_cast<ReincarnationComponent*>(component) };
                                              
                        }
                        catch (...) {
                            g_runtimeApi->ToLogError("Start: Throw to start component(%s).", "ReincarnationComponent");
                            return;
                        } } };
                        start(p);
                        return p;
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Create: Throw to create component(%s).", "ReincarnationComponent");
                        return nullptr;
                    }
                },
                // Destroy
                [](void* component) noexcept {
                    try {
                        if (!component)
                        {
                            g_runtimeApi->ToLogError("Destroy: Null component(%s) pointer.", "ReincarnationComponent");
                            return;
                        }
                        auto onDestroy{ [](void* component) noexcept {
                            try {
                                                  if (!component)
                         {
                             g_runtimeApi->ToLogError("OnDestroy: Null component(%s) pointer.", "ReincarnationComponent");
                             return;
                         }
                         auto c{ static_cast<ReincarnationComponent*>(component) };
                                              
                            }
                            catch (...) {
                                g_runtimeApi->ToLogError("OnDestroy: Throw to onDestroy component(%s).", "ReincarnationComponent");
                                return;
                            } } };
                        onDestroy(component);
                        auto c{ static_cast<ReincarnationComponent*>(component) };
                        delete c;
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Destroy: Throw to destroy component(%s).", "ReincarnationComponent");
                        return;
                    }
                },
                // Copy
                [](void* component) noexcept -> void* {
                    try {
                        if (!component)
                        {
                            g_runtimeApi->ToLogError("Copy: Null component(%s) pointer.", "ReincarnationComponent");
                            return nullptr;
                        }
                        auto c{ static_cast<ReincarnationComponent*>(component) };
                        return new ReincarnationComponent(*c);
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Copy: Throw to copy component(%s).", "ReincarnationComponent");
                        return nullptr;
                    }

                },
                // Serialize
                [](void* component, nlohmann::json& json) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("Serialize: Null component(%s) pointer.", "ReincarnationComponent");
                             return;
                         }
                         auto c{ static_cast<ReincarnationComponent*>(component) };
                                              
                         json["Limit"] = c->m_limit;
                     
                         json["Start"]["x"] = c->m_start.x;
                         json["Start"]["y"] = c->m_start.y;
                         json["Start"]["z"] = c->m_start.z;
                     
                         json["Move"]["x"] = c->m_move.x;
                         json["Move"]["y"] = c->m_move.y;
                         json["Move"]["z"] = c->m_move.z;
                     
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Serialize: Throw to serialize logic(%s).", "Reincarnation");
                        return;
                    }
                },
                // Deserialize
                [](void* component, const nlohmann::json& json) noexcept {
                    try {
                                                  if (!component)
                         {
                             g_runtimeApi->ToLogError("Deserialize: Null component(%s) pointer.", "ReincarnationComponent");
                             return;
                         }
                         auto c{ static_cast<ReincarnationComponent*>(component) };
                                              
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
                    catch (...) {
                        g_runtimeApi->ToLogError("Deserialize: Throw to deserialize logic(%s).", "Reincarnation");
                        return;
                    }
                },
                // RenderEditor
                [](void* component, entityId id) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("RenderEditor: Null component(%s) pointer.", "ReincarnationComponent");
                             return;
                         }
                         auto c{ static_cast<ReincarnationComponent*>(component) };
                                              
                         ImGui::DragFloat("Limit", &c->m_limit);
                         ImGui::DragFloat3("Start Pos", &c->m_start.x);
                         ImGui::DragFloat3("MoveSpeed", &c->m_move.x);
                     
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("RenderEditor: Throw to renderEditor logic(%s).", "Reincarnation");
                        return;
                    }
                },
                // Update
                [](void* component, entityId id, float deltaTime) noexcept {
                    try {
                                                  if(!component)
                         {
                             g_runtimeApi->ToLogError("Update: Null component(%s) pointer.", "ReincarnationComponent");
                             return;
                         }
                         auto c{ static_cast<ReincarnationComponent*>(component) };
                                              
                         auto tc{ static_cast<TransformComponent*>(g_runtimeApi->GetComponent("Transform",id)) };
                         if (!tc) return;
                     
                         if (std::abs(tc->m_transform->GetLocalPosition().x - c->m_start.x) > c->m_limit)
                             tc->m_transform->SetLocalPosition(c->m_start);
                     
                         auto pos{ tc->m_transform->GetLocalPosition() };
                         
                         pos += c->m_move * deltaTime;
                         tc->m_transform->SetLocalPosition(pos);
                     
                     
                    }
                    catch (...) {
                        g_runtimeApi->ToLogError("Update: Throw to update logic(%s).", "Reincarnation");
                        return;
                    }
                }
            };

            g_runtimeApi->RegisterModule("Reincarnation", &reflection);
            g_runtimeApi->ToLogInfo("Regist: %s", "Reincarnation");
            return { FlResult::Fl_OK, __LINE__ };
        }
        else return { FlResult::Fl_FAIL, __LINE__ };
    }
    catch (...) {
        return { FlResult::Fl_THROW, __LINE__ };
    }
}