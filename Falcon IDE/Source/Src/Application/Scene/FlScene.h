#pragma once

#include "../Module/ScriptModuleLoader/FlScriptModuleLoader.h"

class FlScene
{
public:
    void Initializer();
    void PostProcess();

    void Update(float deltaTime);

    const auto& GetScriptModuleLoader() const noexcept { return m_upLoader; }

    static auto& Instance() noexcept
    {
        static auto instance{ FlScene{} };
        return instance;
    }

private:

    FlScene() {
        m_upLoader = std::make_unique<FlScriptModuleLoader>("Src/Framework/Module/ScriptDLLs/");
    }
    std::unique_ptr<FlScriptModuleLoader> m_upLoader;
};