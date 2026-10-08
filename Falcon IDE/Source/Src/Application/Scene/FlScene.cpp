#include "FlScene.h"

#include "../../Core/FlEntityComponentSystemKernel.h"

void FlScene::Initializer()
{
    FlEntityComponentSystemKernel::Instance().initialize();
    const std::filesystem::path path{ "Assets/Scene/lastTime.flscene" };
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    m_canSave = !ec;
    if (exists && m_canSave)
    {
        auto j = nlohmann::json{};
        m_canSave = FlJsonUtility::Deserialize(j, path);
        if (m_canSave)
        {
            try { FlEntityComponentSystemKernel::Instance().DeserializeScene(j); }
            catch (...) { m_canSave = false; }
        }
    }
    if (!m_canSave)
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Scene load failed; automatic save is disabled to preserve %s", path.string().c_str());
    FlEditorAdministrator::Instance().RefreshHierarchy();
}

bool FlScene::PostProcess()
{
    if (!m_canSave) return false;
    try {
        std::filesystem::create_directories("Assets/Scene");
        return FlJsonUtility::Serialize(FlEntityComponentSystemKernel::Instance().SerializeScene(), "Assets/Scene/lastTime.flscene");
    }
    catch (...) { return false; }
}

void FlScene::Update(float deltaTime)
{
    if(FlEditorAdministrator::Instance().GetIsStop()) deltaTime = Def::FloatZero;

    m_upLoader->Update();

    FlEntityComponentSystemKernel::Instance().UpdateAll(deltaTime);
}
