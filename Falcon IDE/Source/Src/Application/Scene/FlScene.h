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
#ifdef _DEBUG
        constexpr auto moduleRoot = "Src/Framework/Module/ScriptDLLs/Debug/";
#else
        constexpr auto moduleRoot = "Src/Framework/Module/ScriptDLLs/Release/";
#endif
        m_upLoader = std::make_unique<FlScriptModuleLoader>(moduleRoot);
    }
    std::unique_ptr<FlScriptModuleLoader> m_upLoader;
};
