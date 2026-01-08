#include "FlDynamicShaderManager.h"
#include "Framework/Graphics/GraphicsDevice.h"
#include "Framework/Resource/Shader/ShaderManager.h"
#include "Framework/ImGui/Editor/FlEditorAdministrator.h"

bool FlDynamicShaderManager::Initialize(GraphicsDevice* device, FlResourceAdministrator* resourceAdmin)
{
    if (!device || !resourceAdmin)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("FlDynamicShaderManager: Invalid device or resource admin");
        return false;
    }

    m_device = device;
    m_resourceAdmin = resourceAdmin;
    m_initialized = true;

    FlEditorAdministrator::Instance().GetLogger()->AddLog("FlDynamicShaderManager initialized successfully");
    return true;
}

bool FlDynamicShaderManager::RegisterShaderConfig(const std::string& configName,
    const std::string& vsPath,
    const std::string& psPath,
    const std::string& hsPath,
    const std::string& dsPath,
    const std::string& gsPath,
    const std::vector<InputLayout>& inputLayouts,
    CullMode cullMode,
    BlendMode blendMode,
    PrimitiveTopologyType topologyType,
    const std::vector<DXGI_FORMAT>& formats,
    bool isDepth,
    bool isDepthMask,
    int rtvCount,
    bool isWireFrame,
    ShaderPresetType presetType)
{
    if (!m_initialized)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("FlDynamicShaderManager not initialized");
        return false;
    }

    // Check if configuration already exists
    if (m_shaderConfigs.find(configName) != m_shaderConfigs.end())
    {
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Shader config '" + configName + "' already exists, overwriting");
    }

    ShaderConfiguration config;
    config.name = configName;
    config.topologyType = topologyType;

    // Load shader blobs
    if (!LoadShaderBlob(vsPath, config.vsBlob))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to load vertex shader: " + vsPath);
        return false;
    }

    if (!LoadShaderBlob(psPath, config.psBlob))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to load pixel shader: " + psPath);
        return false;
    }

    if (!hsPath.empty() && !LoadShaderBlob(hsPath, config.hsBlob))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed to load hull shader: " + hsPath);
    }

    if (!dsPath.empty() && !LoadShaderBlob(dsPath, config.dsBlob))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed to load domain shader: " + dsPath);
    }

    if (!gsPath.empty() && !LoadShaderBlob(gsPath, config.gsBlob))
    {
        FlEditorAdministrator::Instance().GetLogger()->AddWarningLog("Failed to load geometry shader: " + gsPath);
    }

    // Set rendering settings
    config.renderingSetting.Reset();
    config.renderingSetting.InputLayouts = inputLayouts;
    config.renderingSetting.CullMode = cullMode;
    config.renderingSetting.BlendMode = blendMode;
    config.renderingSetting.PrimitiveTopologyType = topologyType;
    config.renderingSetting.Formats = formats;
    config.renderingSetting.IsDepth = isDepth;
    config.renderingSetting.IsDepthMask = isDepthMask;
    config.renderingSetting.RTVCount = rtvCount;
    config.renderingSetting.IsWireFrame = isWireFrame;

    // Reflect shader resources and input layouts
    ReflectShaderResources(config.vsBlob, config);
    ReflectShaderResources(config.psBlob, config);
    if (config.hsBlob) ReflectShaderResources(config.hsBlob, config);
    if (config.dsBlob) ReflectShaderResources(config.dsBlob, config);
    if (config.gsBlob) ReflectShaderResources(config.gsBlob, config);

    ReflectInputLayouts(config.vsBlob, config);

    // Create root signature and pipeline
    CreateRootSignature(config);
    CreatePipeline(config);

    // Create bone transforms for skinned meshes
    if (presetType == ShaderPresetType::SkinnedMesh || config.isSkinnedMesh)
    {
        config.boneTransforms = std::make_unique<CBufferData::BoneTransforms>();
        config.isSkinnedMesh = true;
    }

    // Store configuration
    m_shaderConfigs[configName] = std::move(config);

    // Register preset if specified
    if (presetType != ShaderPresetType::Custom)
    {
        m_presetConfigs[presetType] = configName;
    }

    FlEditorAdministrator::Instance().GetLogger()->AddLog("Registered shader config: " + configName);
    return true;
}

bool FlDynamicShaderManager::RegisterStaticMeshShader(const std::string& configName,
    const std::string& vsPath,
    const std::string& psPath,
    bool hasNormalMap,
    bool hasEmissiveMap,
    bool hasMetallicRoughnessMap)
{
    std::vector<InputLayout> inputLayouts = { InputLayout::POSITION, InputLayout::NORMAL, InputLayout::TEXCOORD };
    
    if (hasNormalMap)
    {
        inputLayouts.push_back(InputLayout::TANGENT);
    }

    ShaderConfiguration& config = m_shaderConfigs[configName];
    config.hasNormalMap = hasNormalMap;
    config.hasEmissiveMap = hasEmissiveMap;
    config.hasMetallicRoughnessMap = hasMetallicRoughnessMap;
    config.isSkinnedMesh = false;

    return RegisterShaderConfig(configName, vsPath, psPath, "", "", "", inputLayouts,
        CullMode::Back, BlendMode::Alpha, PrimitiveTopologyType::Triangle,
        { DXGI_FORMAT_R8G8B8A8_UNORM }, true, true, 1, false, ShaderPresetType::StaticMesh);
}

bool FlDynamicShaderManager::RegisterSkinnedMeshShader(const std::string& configName,
    const std::string& vsPath,
    const std::string& psPath,
    bool hasNormalMap,
    bool hasEmissiveMap,
    bool hasMetallicRoughnessMap)
{
    std::vector<InputLayout> inputLayouts = { 
        InputLayout::POSITION, InputLayout::NORMAL, InputLayout::TEXCOORD,
        InputLayout::SKININDEX, InputLayout::SKINWEIGHT 
    };
    
    if (hasNormalMap)
    {
        inputLayouts.push_back(InputLayout::TANGENT);
    }

    ShaderConfiguration& config = m_shaderConfigs[configName];
    config.hasNormalMap = hasNormalMap;
    config.hasEmissiveMap = hasEmissiveMap;
    config.hasMetallicRoughnessMap = hasMetallicRoughnessMap;
    config.isSkinnedMesh = true;

    return RegisterShaderConfig(configName, vsPath, psPath, "", "", "", inputLayouts,
        CullMode::Back, BlendMode::Alpha, PrimitiveTopologyType::Triangle,
        { DXGI_FORMAT_R8G8B8A8_UNORM }, true, true, 1, false, ShaderPresetType::SkinnedMesh);
}

bool FlDynamicShaderManager::SwitchToShaderConfig(ID3D12GraphicsCommandList* commandList,
    const std::string& configName,
    uint32_t viewportWidth,
    uint32_t viewportHeight)
{
    if (!m_initialized || !commandList)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("FlDynamicShaderManager not initialized or invalid command list");
        return false;
    }

    auto it = m_shaderConfigs.find(configName);
    if (it == m_shaderConfigs.end())
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Shader config not found: " + configName);
        return false;
    }

    const ShaderConfiguration& config = it->second;

    // Get PSO
    ID3D12PipelineState* pso = config.pipeline->GetPipeline(
        config.renderingSetting.BlendMode,
        config.renderingSetting.CullMode,
        config.renderingSetting.PrimitiveTopologyType,
        config.renderingSetting.IsDepth,
        config.renderingSetting.IsDepthMask,
        config.renderingSetting.RTVCount,
        config.renderingSetting.IsWireFrame
    );

    if (!pso)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Failed to get PSO for config: " + configName);
        return false;
    }

    // Set pipeline state and root signature
    commandList->SetPipelineState(pso);
    commandList->SetGraphicsRootSignature(config.rootSignature->GetRootSignature());

    // Set primitive topology
    SetPrimitiveTopology(commandList, config.topologyType);

    // Set viewport and scissor
    SetViewportAndScissor(commandList, viewportWidth, viewportHeight);

    // Update current configuration
    m_currentConfig = &config;
    m_currentConfigName = configName;

    FlEditorAdministrator::Instance().GetLogger()->AddLog("Switched to shader config: " + configName);
    return true;
}

bool FlDynamicShaderManager::SwitchToPresetShader(ID3D12GraphicsCommandList* commandList,
    ShaderPresetType presetType,
    uint32_t viewportWidth,
    uint32_t viewportHeight)
{
    auto it = m_presetConfigs.find(presetType);
    if (it == m_presetConfigs.end())
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Preset shader not found for type: " + std::to_string(static_cast<int>(presetType)));
        return false;
    }

    return SwitchToShaderConfig(commandList, it->second, viewportWidth, viewportHeight);
}

const ShaderConfiguration* FlDynamicShaderManager::GetShaderConfig(const std::string& configName) const
{
    auto it = m_shaderConfigs.find(configName);
    return (it != m_shaderConfigs.end()) ? &it->second : nullptr;
}

bool FlDynamicShaderManager::HasShaderConfig(const std::string& configName) const
{
    return m_shaderConfigs.find(configName) != m_shaderConfigs.end();
}

std::vector<std::string> FlDynamicShaderManager::GetRegisteredConfigNames() const
{
    std::vector<std::string> names;
    names.reserve(m_shaderConfigs.size());
    
    for (const auto& pair : m_shaderConfigs)
    {
        names.push_back(pair.first);
    }
    
    return names;
}

void FlDynamicShaderManager::Cleanup()
{
    m_shaderConfigs.clear();
    m_presetConfigs.clear();
    m_currentConfig = nullptr;
    m_currentConfigName.clear();
    m_initialized = false;
}

bool FlDynamicShaderManager::LoadShaderBlob(const std::string& path, ComPtr<ID3DBlob>& blob)
{
    if (path.empty()) return true;

    // Try to load from resource administrator first
    if (m_resourceAdmin)
    {
        try
        {
            blob = *m_resourceAdmin->Get<ComPtr<ID3DBlob>>(path);
            if (blob)
            {
                FlEditorAdministrator::Instance().GetLogger()->AddLog("Loaded shader blob from resource admin: " + path);
                return true;
            }
        }
        catch (const std::exception& e)
        {
            FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Exception loading from resource admin: " + std::string(e.what()));
        }
    }

    // Fallback: try to load directly from file
    std::wstring wPath(path.begin(), path.end());
    HRESULT hr = D3DReadFileToBlob(wPath.c_str(), &blob);
    
    if (FAILED(hr))
    {
        char errorMsg[256];
        sprintf_s(errorMsg, "Failed to load shader blob: %s (HRESULT: 0x%08X)", path.c_str(), hr);
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog(errorMsg);
        return false;
    }

    FlEditorAdministrator::Instance().GetLogger()->AddLog("Loaded shader blob from file: " + path);
    return true;
}

void FlDynamicShaderManager::ReflectShaderResources(ComPtr<ID3DBlob> shaderBlob, ShaderConfiguration& config)
{
    if (!shaderBlob) return;

    ComPtr<ID3D12ShaderReflection> reflector;
    if (FAILED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflector))))
    {
        return;
    }

    D3D12_SHADER_DESC shaderDesc;
    if (FAILED(reflector->GetDesc(&shaderDesc)))
    {
        return;
    }

    // Reflect bound resources
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
        if (FAILED(reflector->GetResourceBindingDesc(i, &bindDesc)))
        {
            continue;
        }

        switch (bindDesc.Type)
        {
        case D3D_SIT_CBUFFER:
            config.rangeTypes.push_back(RangeType::CBV);
            config.cbvCount++;
            break;
        case D3D_SIT_TEXTURE:
            config.rangeTypes.push_back(RangeType::SRV);
            break;
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
            config.rangeTypes.push_back(RangeType::UAV);
            break;
        default:
            break;
        }
    }

    // Sort range types for consistent ordering
    std::sort(config.rangeTypes.begin(), config.rangeTypes.end(),
        [](RangeType a, RangeType b) { return static_cast<int>(a) < static_cast<int>(b); });
}

void FlDynamicShaderManager::ReflectInputLayouts(ComPtr<ID3DBlob> shaderBlob, ShaderConfiguration& config)
{
    if (!shaderBlob) return;

    ComPtr<ID3D12ShaderReflection> reflector;
    if (FAILED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflector))))
    {
        return;
    }

    D3D12_SHADER_DESC shaderDesc;
    if (FAILED(reflector->GetDesc(&shaderDesc)))
    {
        return;
    }

    // Reflect input parameters
    for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
        if (FAILED(reflector->GetInputParameterDesc(i, &paramDesc)))
        {
            continue;
        }

        // Add input layouts based on semantic names
        if (strcmp(paramDesc.SemanticName, "POSITION") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::POSITION) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::POSITION);
            }
        }
        else if (strcmp(paramDesc.SemanticName, "NORMAL") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::NORMAL) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::NORMAL);
            }
        }
        else if (strcmp(paramDesc.SemanticName, "TEXCOORD") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::TEXCOORD) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::TEXCOORD);
            }
        }
        else if (strcmp(paramDesc.SemanticName, "TANGENT") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::TANGENT) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::TANGENT);
            }
        }
        else if (strcmp(paramDesc.SemanticName, "COLOR") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::COLOR) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::COLOR);
            }
        }
        else if (strcmp(paramDesc.SemanticName, "SKININDEX") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::SKININDEX) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::SKININDEX);
            }
        }
        else if (strcmp(paramDesc.SemanticName, "SKINWEIGHT") == 0 && paramDesc.SemanticIndex == 0)
        {
            if (std::find(config.renderingSetting.InputLayouts.begin(), 
                         config.renderingSetting.InputLayouts.end(), 
                         InputLayout::SKINWEIGHT) == config.renderingSetting.InputLayouts.end())
            {
                config.renderingSetting.InputLayouts.push_back(InputLayout::SKINWEIGHT);
            }
        }
    }
}

void FlDynamicShaderManager::CreateRootSignature(ShaderConfiguration& config)
{
    config.rootSignature = std::make_unique<RootSignature>();
    config.rootSignature->Create(m_device, config.rangeTypes, config.cbvCount);
}

void FlDynamicShaderManager::CreatePipeline(ShaderConfiguration& config)
{
    config.pipeline = std::make_unique<Pipeline>();
    
    // Set render settings
    config.pipeline->SetRenderSettings(m_device, config.rootSignature.get(),
        config.renderingSetting.InputLayouts,
        config.renderingSetting.CullMode,
        config.renderingSetting.BlendMode,
        config.renderingSetting.PrimitiveTopologyType,
        config.renderingSetting.IsWireFrame);

    if (!config.vsBlob)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Vertex shader blob is null for config: " + config.name);
        return;
    }

    if (!config.psBlob)
    {
        FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Pixel shader blob is null for config: " + config.name);
        return;
    }

    // Create pipeline with shader blobs
    std::vector<ComPtr<ID3DBlob>> blobs = {
        config.vsBlob, config.hsBlob, config.dsBlob, config.gsBlob, config.psBlob
    };

    FlEditorAdministrator::Instance().GetLogger()->AddLog("Creating pipeline for config: " + config.name);
    FlEditorAdministrator::Instance().GetLogger()->AddLog("VS: %s", (config.vsBlob ? "OK" : "NULL"));
    FlEditorAdministrator::Instance().GetLogger()->AddLog("PS: %s", (config.psBlob ? "OK" : "NULL"));
    FlEditorAdministrator::Instance().GetLogger()->AddLog("HS: %s", (config.hsBlob ? "OK" : "NULL"));
    FlEditorAdministrator::Instance().GetLogger()->AddLog("DS: %s", (config.dsBlob ? "OK" : "NULL"));
    FlEditorAdministrator::Instance().GetLogger()->AddLog("GS: %s", (config.gsBlob ? "OK" : "NULL"));
    FlEditorAdministrator::Instance().GetLogger()->AddLog("InputLayouts: " + std::to_string(config.renderingSetting.InputLayouts.size()));

    config.pipeline->Create(blobs,
        config.renderingSetting.Formats,
        config.renderingSetting.IsDepth,
        config.renderingSetting.IsDepthMask,
        config.renderingSetting.RTVCount,
        config.renderingSetting.IsWireFrame);
}

void FlDynamicShaderManager::SetPrimitiveTopology(ID3D12GraphicsCommandList* commandList, PrimitiveTopologyType topologyType)
{
    switch (topologyType)
    {
    case PrimitiveTopologyType::Point:
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        break;
    case PrimitiveTopologyType::Line:
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        break;
    case PrimitiveTopologyType::Triangle:
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
    case PrimitiveTopologyType::Patch:
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
        break;
    default:
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
    }
}

void FlDynamicShaderManager::SetViewportAndScissor(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height)
{
    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect = {};
    scissorRect.right = static_cast<LONG>(width);
    scissorRect.bottom = static_cast<LONG>(height);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}
