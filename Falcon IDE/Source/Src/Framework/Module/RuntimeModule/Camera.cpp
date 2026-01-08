#include "ResistCamera.h"
#include "Camera.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

#include "Transform.h"

ResistCamera::ResistCamera()
{
	ComponentReflection r{
        // Create
        []() noexcept -> void* {
            try {
                auto p{ new CameraComponent() };

                if (p->m_aspect == Def::Vec2)
                {
                    p->m_aspect.x = 1920;
                    p->m_aspect.y = 1080;
                }

                if (p->m_clips == Def::Vec2)
                {
                    p->m_clips.x = 0.01f;
                    p->m_clips.y = 1000.0f;
                }

                if (p->m_fov == Def::FloatZero)
                    p->m_fov = 60.0f;

                p->m_mProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(p->m_fov),
                    p->m_aspect.x / p->m_aspect.y, p->m_clips.x, p->m_clips.y);

                return p;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Create: Throw to create component(%s).", "CameraComponent");
                return nullptr;
            }
        },
        // Destroy
        [](void* component) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Null component(%s) pointer.", "CameraComponent");
                    return;
                }
                auto c{ static_cast<CameraComponent*>(component) };
                delete c;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Throw to destroy component(%s).", "CameraComponent");
                return;
            }
        },
        // Copy
        [](void* component) noexcept -> void* {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CameraComponent");
                    return nullptr;
                }
                auto c{ static_cast<CameraComponent*>(component) };
                return new CameraComponent(*c);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Throw to copy component(%s).", "CameraComponent");
                return nullptr;
            }

        },
        // Serialize
        [](void* component, nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CameraComponent");
                    return;
                }
                auto c{ static_cast<CameraComponent*>(component) };

                for (int i = 0; i < 4; ++i) {
                    for (int k = 0; k < 4; ++k) {
                        json["viewMatrix"].push_back(c->m_mView.m[i][k]);
                    }
                }

                for (int i = 0; i < 4; ++i) {
                    for (int k = 0; k < 4; ++k) {
                        json["projMatrix"].push_back(c->m_mProj.m[i][k]);
                    }
                }

                json["cameraTagetId"] = c->m_targetId;

                json["CameraOffset"]["x"] = c->m_offset.x;
                json["CameraOffset"]["y"] = c->m_offset.y;
                json["CameraOffset"]["z"] = c->m_offset.z;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Serialize: Throw to serialize logic(%s).", "Camera");
                return;
            }
                },
        // Deserialize
        [](void* component, const nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CameraComponent");
                    return;
                }
                auto c{ static_cast<CameraComponent*>(component) };
                
                auto index{ Def::UIntZero };
                for (int i = 0; i < 4; ++i) {
                    for (int k = 0; k < 4; ++k) {
                        c->m_mView.m[i][k] = json["viewMatrix"][index++];
                    }
                }

                index = Def::UIntZero;
                for (int i = 0; i < 4; ++i) {
                    for (int k = 0; k < 4; ++k) {
                        c->m_mProj.m[i][k] = json["projMatrix"][index++];
                    }
                }

                if (json.contains("cameraTagetId"))
                {
                    c->m_targetId = json["cameraTagetId"];
                }
                if (json.contains("CameraOffset"))
                {
                    json.at("CameraOffset").at("x").get_to(c->m_offset.x);
                    json.at("CameraOffset").at("y").get_to(c->m_offset.y);
                    json.at("CameraOffset").at("z").get_to(c->m_offset.z);
                }
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Deserialize: Throw to deserialize logic(%s).", "Camera");
                return;
            }
        },
        // RenderEditor
        [](void* component, entityId id) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "CameraComponent");
                    return;
                }
                auto c{ static_cast<CameraComponent*>(component) };
                auto is{ false };

                ImGui::Checkbox("Enable", &c->m_isEnable);

                if (ImGui::DragFloat2("Aspect", &c->m_aspect.x, 1.0f)) is = !is;
                if (ImGui::DragFloat2("Clips", &c->m_clips.x, 0.01f))
                {
                    is = !is;
                    if (c->m_clips.x > c->m_clips.y) std::swap(c->m_clips.x, c->m_clips.y);
                }

                if (ImGui::DragFloat("FOV", &c->m_fov)) is = !is;

                ImGui::InputInt("Camera Taget ID", &c->m_targetId);
                if (c->m_targetId != -Def::IntOne)
                {
                    auto ttc{ static_cast<TransformComponent*>(FlEntityComponentSystemKernel::Instance().GetComponent("Transform",c->m_targetId)) };

                    if (ttc)
                    {
                        auto p{ ttc->m_transform->GetLocalPosition() };
                        ImGui::Text("Target X: %f, Y: %f, Z: %f", p.x, p.y, p.z);
                    }
                }

                ImGui::DragFloat3("Offset", &c->m_offset.x);

                if (is)
                    c->m_mProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(c->m_fov),
                        c->m_aspect.x / c->m_aspect.y, c->m_clips.x, c->m_clips.y);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("RenderEditor: Throw to renderEditor logic(%s).", "Camera");
                return;
            }
        },
        // Update
        [](void* component, entityId id, float deltaTime) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Null component(%s) pointer.", "CameraComponent");
                    return;
                }
                auto c{ static_cast<CameraComponent*>(component) };

                if (!c->m_isEnable) return;

                auto tc{ static_cast<TransformComponent*>(FlEntityComponentSystemKernel::Instance().GetComponent("Transform",id)) };

                if (!tc)return;

                if (c->m_targetId != -Def::IntOne)
                {
                    auto ttc{ static_cast<TransformComponent*>(FlEntityComponentSystemKernel::Instance().GetComponent("Transform",c->m_targetId)) };

                    if (ttc)
                    {
                        const auto targetPos = ttc->m_transform->GetLocalPosition();

                        auto camPos = tc->m_transform->GetLocalPosition();

                        camPos = targetPos + c->m_offset;

                        tc->m_transform->SetLocalPosition(camPos);
                    }
                }
                c->m_mView = tc->m_transform->GetWorldMatrix().Invert();

                CBufferData::Camera cm{ c->m_mView,c->m_mProj };

                GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(0, cm);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Throw to update logic(%s).", "Camera");
                return;
            }
        }
    };
	FlEntityComponentSystemKernel::Instance().RegisterModule("Camera", r, Def::UIntZero);
}