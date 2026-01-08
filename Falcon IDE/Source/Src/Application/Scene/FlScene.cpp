#include "FlScene.h"

#include "../../Core/FlEntityComponentSystemKernel.h"

void FlScene::Initializer()
{
    FlEntityComponentSystemKernel::Instance().initialize();

    auto j{ nlohmann::json{} };
    if (FlJsonUtility::Deserialize(j, "Assets/Scene/lastTime.flscene"))
        FlEntityComponentSystemKernel::Instance().DeserializeScene(j);
    else
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed load scene %s", "Assets/Scene/lastTime.flscene");

    FlEditorAdministrator::Instance().RefreshHierarchy();
}

void FlScene::PostProcess()
{
    FlJsonUtility::Serialize(FlEntityComponentSystemKernel::Instance().SerializeScene(), "Assets/Scene/lastTime.flscene");
}

void FlScene::Update(float deltaTime)
{
    if(FlEditorAdministrator::Instance().GetIsStop()) deltaTime = Def::FloatZero;

    m_upLoader->Update();

    FlEntityComponentSystemKernel::Instance().UpdateAll(deltaTime);
}
