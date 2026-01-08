#pragma once

#include "Pipeline/Pipeline.h"
#include "RootSignature/RootSignature.h"
#include "Shader.h"
#include "Framework/Resource/FlResourceAdministrator.h"
#include "Framework/System/Debugger/Logger/FlDebugLogger.hpp"

// Forward declarations
class GraphicsDevice;
class FlResourceAdministrator;

// Shader configuration structure
struct ShaderConfiguration
{
    std::string name;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> hsBlob;
    ComPtr<ID3DBlob> dsBlob;
    ComPtr<ID3DBlob> gsBlob;
    
    std::unique_ptr<RootSignature> rootSignature;
    std::unique_ptr<Pipeline> pipeline;
    std::unique_ptr<CBufferData::BoneTransforms> boneTransforms;
    
    RenderingSetting renderingSetting;
    PrimitiveTopologyType topologyType;
    
    // Resource bindings
    std::vector<RangeType> rangeTypes;
    UINT cbvCount = 0;
    
    // Material properties
    bool hasNormalMap = false;
    bool hasEmissiveMap = false;
    bool hasMetallicRoughnessMap = false;
    bool isSkinnedMesh = false;
};

// Shader preset types
enum class ShaderPresetType
{
    StaticMesh,
    SkinnedMesh,
    Unlit,
    PBR,
    Custom
};

// Dynamic Shader Manager for switching shaders and PSOs at runtime
class FlDynamicShaderManager
{
public:
    static FlDynamicShaderManager& Instance()
    {
        static FlDynamicShaderManager instance;
        return instance;
    }

    // Initialize the manager
    bool Initialize(GraphicsDevice* device, FlResourceAdministrator* resourceAdmin);

    // Register a shader configuration
    bool RegisterShaderConfig(const std::string& configName,
        const std::string& vsPath,
        const std::string& psPath,
        const std::string& hsPath = "",
        const std::string& dsPath = "",
        const std::string& gsPath = "",
        const std::vector<InputLayout>& inputLayouts = { InputLayout::POSITION, InputLayout::NORMAL, InputLayout::TEXCOORD },
        CullMode cullMode = CullMode::Back,
        BlendMode blendMode = BlendMode::Alpha,
        PrimitiveTopologyType topologyType = PrimitiveTopologyType::Triangle,
        const std::vector<DXGI_FORMAT>& formats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        bool isDepth = true,
        bool isDepthMask = true,
        int rtvCount = 1,
        bool isWireFrame = false,
        ShaderPresetType presetType = ShaderPresetType::Custom);

    // Register preset configurations
    bool RegisterStaticMeshShader(const std::string& configName,
        const std::string& vsPath,
        const std::string& psPath,
        bool hasNormalMap = false,
        bool hasEmissiveMap = false,
        bool hasMetallicRoughnessMap = false);

    bool RegisterSkinnedMeshShader(const std::string& configName,
        const std::string& vsPath,
        const std::string& psPath,
        bool hasNormalMap = false,
        bool hasEmissiveMap = false,
        bool hasMetallicRoughnessMap = false);

    // Switch to a specific shader configuration
    bool SwitchToShaderConfig(ID3D12GraphicsCommandList* commandList, 
        const std::string& configName, 
        uint32_t viewportWidth, 
        uint32_t viewportHeight);

    // Switch to a preset shader type
    bool SwitchToPresetShader(ID3D12GraphicsCommandList* commandList,
        ShaderPresetType presetType,
        uint32_t viewportWidth,
        uint32_t viewportHeight);

    // Get current shader configuration
    const ShaderConfiguration* GetCurrentConfig() const { return m_currentConfig; }
    
    // Get shader configuration by name
    const ShaderConfiguration* GetShaderConfig(const std::string& configName) const;
    
    // Check if configuration exists
    bool HasShaderConfig(const std::string& configName) const;
    
    // Get all registered configuration names
    std::vector<std::string> GetRegisteredConfigNames() const;

    // Cleanup
    void Cleanup();

private:
    FlDynamicShaderManager() = default;
    ~FlDynamicShaderManager() = default;
    FlDynamicShaderManager(const FlDynamicShaderManager&) = delete;
    FlDynamicShaderManager& operator=(const FlDynamicShaderManager&) = delete;

    // Helper functions
    bool LoadShaderBlob(const std::string& path, ComPtr<ID3DBlob>& blob);
    void ReflectShaderResources(ComPtr<ID3DBlob> shaderBlob, ShaderConfiguration& config);
    void ReflectInputLayouts(ComPtr<ID3DBlob> shaderBlob, ShaderConfiguration& config);
    void CreateRootSignature(ShaderConfiguration& config);
    void CreatePipeline(ShaderConfiguration& config);
    void SetPrimitiveTopology(ID3D12GraphicsCommandList* commandList, PrimitiveTopologyType topologyType);
    void SetViewportAndScissor(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height);

    // Member variables
    GraphicsDevice* m_device = nullptr;
    FlResourceAdministrator* m_resourceAdmin = nullptr;
    
    std::unordered_map<std::string, ShaderConfiguration> m_shaderConfigs;
    std::unordered_map<ShaderPresetType, std::string> m_presetConfigs;
    
    const ShaderConfiguration* m_currentConfig = nullptr;
    std::string m_currentConfigName;
    
    bool m_initialized = false;
};
