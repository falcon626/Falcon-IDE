#include "NameAndTag.h"
#include "ResistNameAndTag.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

ResistNameAndTag::ResistNameAndTag()
{
	ComponentReflection r{
        // Create
        []() noexcept -> void* {
            try {
                auto p{new NameComponent()};
                p->m_name = "New Entity";
                p->m_tag  = "None";
                return p;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Create: Throw to create component(%s).", "NameComponent");
                return nullptr;
            }
        },
        // Destroy
        [](void* component) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Null component(%s) pointer.", "NameComponent");
                    return;
                }
                auto c{ static_cast<NameComponent*>(component) };
                delete c;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Throw to destroy component(%s).", "NameComponent");
                return;
            }
        },
        // Copy
        [](void* component) noexcept -> void* {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "NameComponent");
                    return nullptr;
                }
                auto c{ static_cast<NameComponent*>(component) };
                return new NameComponent(*c);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Throw to copy component(%s).", "NameComponent");
                return nullptr;
            }

        },
        // Serialize
        [](void* component, nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "NameComponent");
                    return;
                }
                auto c{ static_cast<NameComponent*>(component) };

                json["Name"] = c->m_name;
                json["Tag"]  = c->m_tag;
                json["GUID"] = c->m_guid.ToString();
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Serialize: Throw to serialize logic(%s).", "Name");
                return;
            }
                },
        // Deserialize
        [](void* component, const nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "NameComponent");
                    return;
                }
                auto c{ static_cast<NameComponent*>(component) };

                FlJsonUtility::GetValue(json, "Name", &c->m_name);
                FlJsonUtility::GetValue(json, "Tag", &c->m_tag);
                auto s{ std::string{} };
                FlJsonUtility::GetValue(json, "GUID", &s);
                c->m_guid.FromString(s);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Deserialize: Throw to deserialize logic(%s).", "Name");
                return;
            }
        },
        // RenderEditor
        [](void* component, entityId id) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "NameComponent");
                    return;
                }
                auto c{ static_cast<NameComponent*>(component) };
                
                ImGui::InputText("Entity Name", &c->m_name);
                ImGui::InputText("Tag", &c->m_tag);
                auto sg{ c->m_guid.ToString() };
                auto strGuid{ "GUID: " + sg };
                if (ImGui::Button(strGuid.c_str()))
                    ImGui::SetClipboardText(sg.c_str());
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("RenderEditor: Throw to renderEditor logic(%s).", "Name");
                return;
            }
        },
        // Update
        {}
    };
    FlEntityComponentSystemKernel::Instance().RegisterModule("Name", r);
}