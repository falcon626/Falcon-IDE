#include "Collision.h"
#include "ResistCollision.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

#include "Transform.h"

ResistCollision::ResistCollision()
{
	ComponentReflection r{
        // Create
        []() noexcept -> void* {
            try {
                auto p{ new CollisionComponent() };
                return p;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Create: Throw to create component(%s).", "CollisionComponent");
                return nullptr;
            }
        },
        // Destroy
        [](void* component) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Null component(%s) pointer.", "CollisionComponent");
                    return;
                }
                auto c{ static_cast<CollisionComponent*>(component) };
                delete c;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Throw to destroy component(%s).", "CollisionComponent");
                return;
            }
        },
        // Copy
        [](void* component) noexcept -> void* {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CollisionComponent");
                    return nullptr;
                }
                auto c{ static_cast<CollisionComponent*>(component) };
                return new CollisionComponent(*c);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Throw to copy component(%s).", "CollisionComponent");
                return nullptr;
            }

        },
        // Serialize
        [](void* component, nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CollisionComponent");
                    return;
                }
                auto c{ static_cast<CollisionComponent*>(component) };

                auto type{ std::to_underlying(c->m_collType) };
                json["Type"] = type;

                json["Radius"] = c->m_radius;

                json["BoxSize"]["x"] = c->m_boxSize.x;
                json["BoxSize"]["y"] = c->m_boxSize.y;
                json["BoxSize"]["z"] = c->m_boxSize.z;

                json["RayDir"]["x"] = c->m_rayDir.x;
                json["RayDir"]["y"] = c->m_rayDir.y;
                json["RayDir"]["z"] = c->m_rayDir.z;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Serialize: Throw to serialize logic(%s).", "Collision");
                return;
            }
                },
        // Deserialize
        [](void* component, const nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CollisionComponent");
                    return;
                }
                auto c{ static_cast<CollisionComponent*>(component) };

                auto type{ Def::IntZero };
                FlJsonUtility::GetValue(json, "Type", &type);
                c->m_collType = static_cast<FlCollisionShape::Type>(type);

                FlJsonUtility::GetValue(json, "Radius", &c->m_radius);

                if (json.contains("BoxSize"))
                {
                    json.at("BoxSize").at("x").get_to(c->m_boxSize.x);
                    json.at("BoxSize").at("y").get_to(c->m_boxSize.y);
                    json.at("BoxSize").at("z").get_to(c->m_boxSize.z);
                }
                if (json.contains("RayDir"))
                {
                    json.at("RayDir").at("x").get_to(c->m_rayDir.x);
                    json.at("RayDir").at("y").get_to(c->m_rayDir.y);
                    json.at("RayDir").at("z").get_to(c->m_rayDir.z);
                }
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Deserialize: Throw to deserialize logic(%s).", "Collision");
                return;
            }
        },
        // RenderEditor
        [](void* component, entityId id) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CollisionComponent");
                    return;
                }
                auto c{ static_cast<CollisionComponent*>(component) };

                ImGui::Text((c->m_isHit ? "Hit" : "No Hit"));

                if (ImGui::RadioButton("Box", c->m_collType == FlCollisionShape::Type::Box)) {
                    c->m_collType = FlCollisionShape::Type::Box;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Sphere", c->m_collType == FlCollisionShape::Type::Sphere)) {
                    c->m_collType = FlCollisionShape::Type::Sphere;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Ray", c->m_collType == FlCollisionShape::Type::Ray)) {
                    c->m_collType = FlCollisionShape::Type::Ray;
                }

                switch (c->m_collType) {
                case FlCollisionShape::Type::Box:
                    ImGui::DragFloat3("Box Size", &c->m_boxSize.x);
                    break;
                case FlCollisionShape::Type::Sphere:
                    ImGui::DragFloat("Sphere Radius", &c->m_radius);
                    break;
                case FlCollisionShape::Type::Ray:
                    ImGui::DragFloat3("Ray Direction", &c->m_rayDir.x);
                    break;
                }
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("RenderEditor: Throw to renderEditor logic(%s).", "Collision");
                return;
            }
        },
        // Update
        [](void* component, entityId id, float deltaTime) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Null component(%s) pointer.", "CollisionComponent");
                    return;
                }
                auto c{ static_cast<CollisionComponent*>(component) };

                auto pos{ Def::Vec3 };
                if (auto tc{ static_cast<TransformComponent*>(FlEntityComponentSystemKernel::Instance().
                        GetComponent("Transform", id)) })
                    pos = tc->m_transform->GetLocalPosition();

                switch (c->m_collType) {
                case FlCollisionShape::Type::Box:
                    c->m_collision = FlCollisionShape::CreateBox(pos, c->m_boxSize);
                    break;
                case FlCollisionShape::Type::Sphere:
                    c->m_collision = FlCollisionShape::CreateSphere(pos, c->m_radius);
                    break;
                case FlCollisionShape::Type::Ray:
                    c->m_collision = FlCollisionShape::CreateRay(pos, c->m_rayDir);
                    break;
                }

                c->m_isHit = false;

                for (auto& entList : FlEntityComponentSystemKernel::Instance().GetAllEntityIds())
                {
                    if (FlEntityComponentSystemKernel::Instance().HasComponent("Collision", entList))
                    {
                        if (id == entList) continue;
                        auto cc{ static_cast<CollisionComponent*>(FlEntityComponentSystemKernel::Instance().GetComponent("Collision", entList)) };
                        if (c->m_collision.Intersects(cc->m_collision))
                        {
                            c->m_isHit = true;
                            break;
                        }
                    }
                }

            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Throw to update logic(%s).", "Collision");
                return;
            }
        }
	};
	FlEntityComponentSystemKernel::Instance().RegisterModule("Collision", r);
}