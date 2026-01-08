#include "ModelRender.h"
#include "ResistModelRender.h"
#include "../../Core/FlEntityComponentSystemKernel.h"

#include "Transform.h"

static void DrawNodeRecursive(const std::vector<ModelData::Node>& nodes, int32_t nodeIndex) {
    if (nodeIndex == -Def::IntOne) return;

    const auto& node = nodes[nodeIndex];

    // 1. ノード名とフラグの準備
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (node.m_children.empty()) {
        // 子ノードがない場合は、展開矢印を表示しない
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // 現在のノード名をツリーノードとして表示
    bool is_open = ImGui::TreeNodeEx(
        (void*)(intptr_t)nodeIndex, // ユニークなIDとしてインデックスを使用
        flags,
        "%s (Index: %d, Parent: %d)",
        node.m_name.c_str(),
        node.m_nodeIndex,
        node.m_parentIndex
    );

    // 2. ノードが展開された場合の処理
    if (is_open) {
        // ノードの追加情報を表示 (例: メッシュ情報や行列)
        ImGui::Text("Mesh Ptr: %p", node.m_spMesh.get());

        // 3. 子ノードの描画 (再帰呼び出し)
        for (int32_t childIndex : node.m_children) {
            DrawNodeRecursive(nodes, childIndex);
        }

        // 葉ノードではない場合のみ、Pop処理が必要
        if (!node.m_children.empty()) {
            ImGui::TreePop();
        }
    }
    // ノードが展開されなかった場合の処理（Leafではない場合）
    else if (node.m_children.empty()) {
        // Leafノードの場合は、TreeNodeEx()で既にPop処理が不要になっている
    }
}

ResistModelRender::ResistModelRender()
{
    ComponentReflection r{
        // Create
        []() noexcept -> void* {
            try {
                auto p{new ModelRenderComponent()};
                return p;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Create: Throw to create component(%s).",
                    "ModelRenderComponent");
                return nullptr;
            }
        },
        // Destroy
        [](void* component) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Null component(%s) pointer.", "ModelRenderComponent");
                    return;
                }
                auto c{ static_cast<ModelRenderComponent*>(component) };
                delete c;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Destroy: Throw to destroy component(%s).", "ModelRenderComponent");
                return;
            }
        },
        // Copy
        [](void* component) noexcept -> void* {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "ModelRenderComponent");
                    return nullptr;
                }
                auto c{ static_cast<ModelRenderComponent*>(component) };
                return new ModelRenderComponent(*c);
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Throw to copy component(%s).", "ModelRenderComponent");
                return nullptr;
            }

        },
        // Serialize
        [](void* component, nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "ModelRenderComponent");
                    return;
                }
                auto c{ static_cast<ModelRenderComponent*>(component) };
                
                json["Model"] = c->m_path;
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Serialize: Throw to serialize logic(%s).", "ModelRender");
                return;
            }
                },
        // Deserialize
        [](void* component, const nlohmann::json& json) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "ModelRenderComponent");
                    return;
                }
                auto c{ static_cast<ModelRenderComponent*>(component) };

                FlJsonUtility::GetValue(json, "Model", &c->m_path);

                if (c->m_path.empty()) return;

                if (auto sp{ FlResourceAdministrator::Instance().Get<ModelData>(c->m_path) })
                    c->m_spModel = sp;
                else if (auto sp{ FlResourceAdministrator::Instance().GetByGuid<ModelData>(c->m_path) })
                    c->m_spModel = sp;
                else FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed Load Model %s", c->m_path.c_str());
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Deserialize: Throw to deserialize logic(%s).", "ModelRender");
                return;
            }
        },
        // RenderEditor
        [](void* component, entityId id) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Copy: Null component(%s) pointer.", "ModelRenderComponent");
                    return;
                }
                auto c{ static_cast<ModelRenderComponent*>(component) };

                if (ImGui::InputText("Model", &c->m_path, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    if (c->m_path.empty()) return;

                    if (auto sp{ FlResourceAdministrator::Instance().Get<ModelData>(c->m_path) })
                        c->m_spModel = sp;
                    else if (auto sp{ FlResourceAdministrator::Instance().GetByGuid<ModelData>(c->m_path) })
                        c->m_spModel = sp;
                    else FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed Load Model %s", c->m_path.c_str());
                }

                if (c->m_spModel)
                {
                    auto kNodes{ c->m_spModel->GetNodes() };
                    if (kNodes.empty()) return;

                    for (int32_t i = 0; i < kNodes.size(); ++i) {
                        // ルートノードを特定（親インデックスが初期値 -Def::IntOne のもの）
                        if (kNodes[i].m_parentIndex == -Def::IntOne) {
                            // ルートノードから再帰処理を開始
                            DrawNodeRecursive(kNodes, i);
                        }
                    }
                }
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("RenderEditor: Throw to renderEditor logic(%s).", "ModelRender");
                return;
            }
        },
        // Update
        [](void* component, entityId id, float deltaTime) noexcept {
            try {
                if (!component)
                {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Null component(%s) pointer.", "ModelRenderComponent");
                    return;
                }
                auto c{ static_cast<ModelRenderComponent*>(component) };
                auto tc{ FlEntityComponentSystemKernel::Instance().GetComponent("Transform", id) };

                if (!tc) return;

                auto tcp{ static_cast<TransformComponent*>(tc) };

                if (c->m_path.empty() || !c->m_spModel) return;

                Shader::Instance().DrawModel(*c->m_spModel,
                    tcp->m_transform->GetWorldMatrix());
            }
            catch (...) {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Update: Throw to update logic(%s).", "ModelRender");
                return;
            }
        }
    };
    FlEntityComponentSystemKernel::Instance().RegisterModule("ModelRender", r, Def::UIntOne);
}