#pragma once

#include "FlDynamicShaderManager.h"

// Example usage of FlDynamicShaderManager
class FlDynamicShaderManagerExample
{
public:
    // Initialize the dynamic shader manager
    static bool InitializeExample(GraphicsDevice* device, FlResourceAdministrator* resourceAdmin)
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        if (!shaderManager.Initialize(device, resourceAdmin))
        {
            return false;
        }

        // Register static mesh shaders
        RegisterStaticMeshShaders();
        
        // Register skinned mesh shaders
        RegisterSkinnedMeshShaders();
        
        // Register custom shaders
        RegisterCustomShaders();
        
        return true;
    }

    // Example of registering static mesh shaders
    static void RegisterStaticMeshShaders()
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        // Basic static mesh shader (PBR)
        shaderManager.RegisterStaticMeshShader(
            "StaticMesh_PBR",
            "Shaders/StaticMesh_VS.hlsl",
            "Shaders/PBR_PS.hlsl",
            true,   // hasNormalMap
            true,   // hasEmissiveMap
            true    // hasMetallicRoughnessMap
        );

        // Basic static mesh shader (Unlit)
        shaderManager.RegisterStaticMeshShader(
            "StaticMesh_Unlit",
            "Shaders/StaticMesh_VS.hlsl",
            "Shaders/Unlit_PS.hlsl",
            false,  // hasNormalMap
            false,  // hasEmissiveMap
            false   // hasMetallicRoughnessMap
        );

        // Wireframe shader for debugging
        shaderManager.RegisterShaderConfig(
            "StaticMesh_Wireframe",
            "Shaders/StaticMesh_VS.hlsl",
            "Shaders/Wireframe_PS.hlsl",
            "", "", "", // No tessellation or geometry shaders
            { InputLayout::POSITION, InputLayout::NORMAL, InputLayout::TEXCOORD },
            CullMode::None,      // No culling for wireframe
            BlendMode::Opaque,
            PrimitiveTopologyType::Triangle,
            { DXGI_FORMAT_R8G8B8A8_UNORM },
            true, true, 1,
            true,       // Wireframe mode
            ShaderPresetType::StaticMesh
        );
    }

    // Example of registering skinned mesh shaders
    static void RegisterSkinnedMeshShaders()
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        // Basic skinned mesh shader (PBR)
        shaderManager.RegisterSkinnedMeshShader(
            "SkinnedMesh_PBR",
            "Shaders/SkinnedMesh_VS.hlsl",
            "Shaders/PBR_PS.hlsl",
            true,   // hasNormalMap
            true,   // hasEmissiveMap
            true    // hasMetallicRoughnessMap
        );

        // Basic skinned mesh shader (Unlit)
        shaderManager.RegisterSkinnedMeshShader(
            "SkinnedMesh_Unlit",
            "Shaders/SkinnedMesh_VS.hlsl",
            "Shaders/Unlit_PS.hlsl",
            false,  // hasNormalMap
            false,  // hasEmissiveMap
            false   // hasMetallicRoughnessMap
        );

        // Skinned mesh with tessellation
        shaderManager.RegisterShaderConfig(
            "SkinnedMesh_Tessellated",
            "Shaders/SkinnedMesh_VS.hlsl",
            "Shaders/PBR_PS.hlsl",
            "Shaders/Tessellation_HS.hlsl",  // Hull shader
            "Shaders/Tessellation_DS.hlsl",  // Domain shader
            "",                              // No geometry shader
            { InputLayout::POSITION, InputLayout::NORMAL, InputLayout::TEXCOORD,
              InputLayout::SKININDEX, InputLayout::SKINWEIGHT },
            CullMode::Back,
            BlendMode::Alpha,
            PrimitiveTopologyType::Patch,    // Patch topology for tessellation
            { DXGI_FORMAT_R8G8B8A8_UNORM },
            true, true, 1, false,
            ShaderPresetType::SkinnedMesh
        );
    }

    // Example of registering custom shaders
    static void RegisterCustomShaders()
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        // Custom shader with geometry shader
        shaderManager.RegisterShaderConfig(
            "Custom_GeometryShader",
            "Shaders/Custom_VS.hlsl",
            "Shaders/Custom_PS.hlsl",
            "", "",                           // No tessellation shaders
            "Shaders/Custom_GS.hlsl",        // Geometry shader
            { InputLayout::POSITION, InputLayout::NORMAL, InputLayout::TEXCOORD },
            CullMode::Back,
            BlendMode::Add,                  // Additive blending
            PrimitiveTopologyType::Triangle,
            { DXGI_FORMAT_R8G8B8A8_UNORM },
            true, true, 1, false,
            ShaderPresetType::Custom
        );

        // Custom shader for particles
        shaderManager.RegisterShaderConfig(
            "Particle_Shader",
            "Shaders/Particle_VS.hlsl",
            "Shaders/Particle_PS.hlsl",
            "", "", "",
            { InputLayout::POSITION, InputLayout::COLOR, InputLayout::TEXCOORD },
            CullMode::None,                  // No culling for particles
            BlendMode::Add,                  // Additive blending
            PrimitiveTopologyType::Point,    // Point topology for particles
            { DXGI_FORMAT_R8G8B8A8_UNORM },
            true, true, 1, false,
            ShaderPresetType::Custom
        );
    }

    // Example of switching shaders during rendering
    static void RenderExample(ID3D12GraphicsCommandList* commandList, uint32_t viewportWidth, uint32_t viewportHeight)
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        // Switch to static mesh shader and render static meshes
        if (shaderManager.SwitchToShaderConfig(commandList, "StaticMesh_PBR", viewportWidth, viewportHeight))
        {
            // Render static meshes here
            // DrawMesh(staticMesh);
        }

        // Switch to skinned mesh shader and render animated meshes
        if (shaderManager.SwitchToShaderConfig(commandList, "SkinnedMesh_PBR", viewportWidth, viewportHeight))
        {
            // Render skinned meshes here
            // DrawSkinnedMesh(skinnedMesh);
        }

        // Switch to wireframe shader for debugging
        if (shaderManager.SwitchToShaderConfig(commandList, "StaticMesh_Wireframe", viewportWidth, viewportHeight))
        {
            // Render wireframe meshes here
            // DrawWireframeMesh(mesh);
        }

        // Switch to preset shader types
        if (shaderManager.SwitchToPresetShader(commandList, ShaderPresetType::StaticMesh, viewportWidth, viewportHeight))
        {
            // Render with static mesh preset
        }

        if (shaderManager.SwitchToPresetShader(commandList, ShaderPresetType::SkinnedMesh, viewportWidth, viewportHeight))
        {
            // Render with skinned mesh preset
        }
    }

    // Example of getting current shader information
    static void PrintCurrentShaderInfo()
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        const auto* currentConfig = shaderManager.GetCurrentConfig();
        if (currentConfig)
        {
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Current shader: " + currentConfig->name);
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Is skinned mesh: " + std::to_string(currentConfig->isSkinnedMesh));
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Has normal map: " + std::to_string(currentConfig->hasNormalMap));
            FlEditorAdministrator::Instance().GetLogger()->AddLog("CBV count: " + std::to_string(currentConfig->cbvCount));
        }
    }

    // Example of listing all registered shaders
    static void ListRegisteredShaders()
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        auto configNames = shaderManager.GetRegisteredConfigNames();
        FlEditorAdministrator::Instance().GetLogger()->AddLog("Registered shader configurations:");
        
        for (const auto& name : configNames)
        {
            FlEditorAdministrator::Instance().GetLogger()->AddLog("  - " + name);
        }
    }

    // Example of checking if a shader configuration exists
    static bool CheckShaderExists(const std::string& configName)
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        return shaderManager.HasShaderConfig(configName);
    }

    // Example of getting shader configuration details
    static void PrintShaderConfigDetails(const std::string& configName)
    {
        auto& shaderManager = FlDynamicShaderManager::Instance();
        
        const auto* config = shaderManager.GetShaderConfig(configName);
        if (config)
        {
            FlEditorAdministrator::Instance().GetLogger()->AddLog("Shader config details for: " + configName);
            FlEditorAdministrator::Instance().GetLogger()->AddLog("  Topology type: " + std::to_string(static_cast<int>(config->topologyType)));
            FlEditorAdministrator::Instance().GetLogger()->AddLog("  Cull mode: " + std::to_string(static_cast<int>(config->renderingSetting.CullMode)));
            FlEditorAdministrator::Instance().GetLogger()->AddLog("  Blend mode: " + std::to_string(static_cast<int>(config->renderingSetting.BlendMode)));
            FlEditorAdministrator::Instance().GetLogger()->AddLog("  Input layout count: " + std::to_string(config->renderingSetting.InputLayouts.size()));
            FlEditorAdministrator::Instance().GetLogger()->AddLog("  Range types count: " + std::to_string(config->rangeTypes.size()));
        }
        else
        {
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Shader config not found: " + configName);
        }
    }
};
