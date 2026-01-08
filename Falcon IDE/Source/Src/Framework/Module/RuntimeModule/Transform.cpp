#include "Transform.h"
#include "ResistTransform.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

ResistTransform::ResistTransform()
{
    ComponentReflection r{
        // Create
        []() noexcept -> void* {
            try {
                auto p{new TransformComponent()};
                p->m_transform = std::make_shared<FlTransform>();
                return p;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Create: Throw to create component(%s).",
                    "TransformComponent");
                return nullptr;
            }
        },
        // Destroy
        [](void* component) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Null component(%s) pointer.", "TransformComponent");
                    return;
                }
                auto c{ static_cast<TransformComponent*>(component) };
                c->m_parent = UINT32_MAX;
                c->m_children.clear();
                delete c;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Throw to destroy component(%s).", "TransformComponent");
                return;
            }
        },
        // Copy
        [](void* component) noexcept -> void* {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "TransformComponent");
                    return nullptr;
                }
                auto c{ static_cast<TransformComponent*>(component) };
                return new TransformComponent(*c);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Throw to copy component(%s).", "TransformComponent");
                return nullptr;
            }

        },
        // Serialize
        [](void* component, nlohmann::json& json) noexcept {
            try {
                    if (!component)
                    {
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "TransformComponent");
                        return;
                    }
                    auto c{ static_cast<TransformComponent*>(component) };

                    auto pos{ c->m_transform->GetLocalPosition() };
                    auto rot{ c->m_transform->GetLocalRotation().ToEuler() }; // Quaternion → Euler
                    auto scl{ c->m_transform->GetLocalScale() };

                    json["position"]["x"] = pos.x;
                    json["position"]["y"] = pos.y;
                    json["position"]["z"] = pos.z;

                    json["rotation"]["x"] = rot.x;
                    json["rotation"]["y"] = rot.y;
                    json["rotation"]["z"] = rot.z;

                    json["scale"]["x"] = scl.x;
                    json["scale"]["y"] = scl.y;
                    json["scale"]["z"] = scl.z;

                    json["parent"] = c->m_parent;
                    json["children"] = nlohmann::json::array();
                    for (auto ch : c->m_children)
                        json["children"].push_back(ch);
            }
            catch (...) {
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Serialize: Throw to serialize logic(%s).", "Transform");
                        return;
            }
                },
        // Deserialize
        [](void* component, const nlohmann::json& json) noexcept {
            try {
                    if (!component)
                    {
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "TransformComponent");
                        return;
                    }
                    auto c{ static_cast<TransformComponent*>(component) };

                    float posX, posY, posZ;
                    float rotX, rotY, rotZ;
                    float sclX, sclY, sclZ;

                    json.at("position").at("x").get_to(posX);
                    json.at("position").at("y").get_to(posY);
                    json.at("position").at("z").get_to(posZ);

                    json.at("rotation").at("x").get_to(rotX);
                    json.at("rotation").at("y").get_to(rotY);
                    json.at("rotation").at("z").get_to(rotZ);

                    json.at("scale").at("x").get_to(sclX);
                    json.at("scale").at("y").get_to(sclY);
                    json.at("scale").at("z").get_to(sclZ);

                    c->m_transform->SetLocalPosition(Math::Vector3(posX, posY, posZ));
                    c->m_transform->SetLocalRotation(Math::Quaternion::CreateFromYawPitchRoll(rotY, rotX, rotZ));
                    c->m_transform->SetLocalScale(Math::Vector3(sclX, sclY, sclZ));

                    if (json.contains("parent"))
                        c->m_parent = json["parent"].get<uint32_t>();

                    c->m_children.clear();
                    if (json.contains("children"))
                    {
                        for (auto& ch : json["children"])
                            c->m_children.push_back(ch.get<uint32_t>());
                    }

                    }
                    catch (...) {
                        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Deserialize: Throw to deserialize logic(%s).", "Transform");
                        return;
                    }
                },
        // RenderEditor
        [](void* component, entityId id) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "TransformComponent");
                    return;
                }
                auto c{ static_cast<TransformComponent*>(component) };


                auto pos{ c->m_transform->GetLocalPosition() };
                auto rot{ c->m_transform->GetLocalRotation() };
                auto scl{ c->m_transform->GetLocalScale() };

                if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                    c->m_transform->SetLocalPosition(pos);

                // ユーザー入力値を保持する静的変数
                static Math::Vector3 userEulerDegrees(0, 0, 0);
                static bool isInitialized{ false };

                // 初回のみ現在の回転値で初期化
                if (!isInitialized)
                {
                    auto currentEuler = rot.ToEuler();
                    userEulerDegrees.x = DirectX::XMConvertToDegrees(currentEuler.x);
                    userEulerDegrees.y = DirectX::XMConvertToDegrees(currentEuler.y);
                    userEulerDegrees.z = DirectX::XMConvertToDegrees(currentEuler.z);
                    isInitialized = true;
                }

                if (ImGui::DragFloat3("Rotation", &userEulerDegrees.x, 1.0f))
                {
                    auto normalizeAngle{ [](float angle) -> float {
                        while (angle > 180.0f) angle -= 360.0f;
                        while (angle < -180.0f) angle += 360.0f;
                        return angle;
                        } };

                    userEulerDegrees.x = normalizeAngle(userEulerDegrees.x);
                    userEulerDegrees.y = normalizeAngle(userEulerDegrees.y);
                    userEulerDegrees.z = normalizeAngle(userEulerDegrees.z);

                    // 度数法からラジアンに変換
                    Math::Vector3 eulerRadians(
                        DirectX::XMConvertToRadians(userEulerDegrees.x),
                        DirectX::XMConvertToRadians(userEulerDegrees.y),
                        DirectX::XMConvertToRadians(userEulerDegrees.z)
                    );

                    // シンプルなクォータニオン作成
                    auto qx{ Math::Quaternion::CreateFromAxisAngle(Math::Vector3::UnitX, eulerRadians.x) };
                    auto qy{ Math::Quaternion::CreateFromAxisAngle(Math::Vector3::UnitY, eulerRadians.y) };
                    auto qz{ Math::Quaternion::CreateFromAxisAngle(Math::Vector3::UnitZ, eulerRadians.z) };

                    auto q{ qz * qy * qx };
                    q.Normalize();

                    c->m_transform->SetLocalRotation(q);
                }

                if (ImGui::DragFloat3("Scale", &scl.x, 0.1f))
                    c->m_transform->SetLocalScale(scl);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("RenderEditor: Throw to renderEditor logic(%s).", "Transform");
                return;
            }
                },
        // Update
        [](void* component, entityId id, float deltaTime) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Null component(%s) pointer.", "TransformComponent");
                    return;
                }
                auto c{ static_cast<TransformComponent*>(component) };
                auto tf{ c->m_transform };

                auto& ecs{ FlEntityComponentSystemKernel::Instance() };

                // --- 親設定 ---
                if (c->m_parent != UINT32_MAX)
                {
                    auto parentTC{ static_cast<TransformComponent*>(ecs.GetComponent("Transform", c->m_parent)) };
                    if (!parentTC) return;
                    if (parentTC->m_transform) tf->SetParent(parentTC->m_transform);
                }
                else tf->SetParent(nullptr);

                // --- 子設定 ---
                for (auto& childID : c->m_children)
                {
                    if (!ecs.HasComponent("Transform", childID))
                    {
                        auto it{ std::find(c->m_children.begin(),c->m_children.end(),childID) };
                        c->m_children.erase(it);
                        continue;
                    }

                    auto child{ static_cast<TransformComponent*>(ecs.GetComponent("Transform", childID)) };
                    tf->AddChild(child->m_transform);
                }

                c->m_transform->GetWorldMatrix();
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Throw to update logic(%s).", "Transform");
                return;
            }
        }
    };
    FlEntityComponentSystemKernel::Instance().RegisterModule("Transform", r, Def::UIntOne + Def::UIntOne);
}