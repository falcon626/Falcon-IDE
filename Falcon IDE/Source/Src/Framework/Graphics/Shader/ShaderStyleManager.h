#pragma once

// シェーダスタイル切り替えを管理するクラス（スタティック/スキンメッシュ用など別構成を保持）
class FlShaderStyleManager {
public:
    FlShaderStyleManager(GraphicsDevice* device, FlResourceAdministrator* readmin) : m_device(device), m_blobManager(readmin) {
        // 既存のShaderManagerインスタンスを取得（シングルトンやグローバル前提）
    }

    // シェーダ構成を登録
    // configName: 構成名（"StaticMesh" など）
    // vsPath, psPath: VS/PSファイルパス（.hlsl or .cso）
    // hsPath, dsPath, gsPath: オプションのHS/DS/GSパス（空文字列で無効）
    // inputLayouts: 入力レイアウト（デフォルト: スタティック用）
    // cullMode, blendMode, topologyType, formats, isDepth, isDepthMask, rtvCount, isWireFrame: 描画設定
    void RegisterShaderConfig(const std::string& configName,
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
        bool isWireFrame = false) {
        ShaderConfig config;
        config.topologyType = topologyType;  // Switch時に使用するため保存

        // Blob取得（既存のShaderManager使用、フライウェイトでロード/共有）
        config.vsBlob = *m_blobManager->Get<ComPtr<ID3DBlob>>(vsPath);
        config.psBlob = *m_blobManager->Get<ComPtr<ID3DBlob>>(psPath);
        config.hsBlob = hsPath.empty() ? nullptr : *m_blobManager->Get<ComPtr<ID3DBlob>>(hsPath);
        config.dsBlob = dsPath.empty() ? nullptr : *m_blobManager->Get<ComPtr<ID3DBlob>>(dsPath);
        config.gsBlob = gsPath.empty() ? nullptr : *m_blobManager->Get<ComPtr<ID3DBlob>>(gsPath);

        // rangeTypesとcbvCountの決定（Shader::Createを模倣）
        UINT cbvCount = 0;
        std::vector<RangeType> rangeTypes;
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

        ReflectShaderCBuffers(config.vsBlob, config.renderingSetting);  // VSから入力レイアウト反映

        std::vector<ComPtr<ID3DBlob>> blobs = { config.vsBlob, config.hsBlob, config.dsBlob, config.gsBlob, config.psBlob };
        for (auto& blob : blobs) {
            if (blob) ShaderView(blob, rangeTypes);  // rangeTypes追加
        }

        // RootSignature作成
        config.rootSignature = std::make_unique<RootSignature>();
        config.rootSignature->Create(m_device, rangeTypes, cbvCount);

        // Pipeline作成
        config.pipeline = std::make_unique<Pipeline>();
        config.pipeline->SetRenderSettings(m_device, config.rootSignature.get(), inputLayouts, cullMode, blendMode, topologyType, isWireFrame);
        config.pipeline->Create(blobs, formats, isDepth, isDepthMask, rtvCount, isWireFrame);

        // BoneTransforms（スキン用、Switch時にセットアップ）
        config.boneTransforms = std::make_unique<CBufferData::BoneTransforms>();

        m_shaderConfigs[configName] = std::move(config);
    }

    // シェーダ構成を動的に切り替え
    // commandList: コマンドリスト
    // configName: 切り替える構成名
    // viewportWidth, viewportHeight: ビューポートサイズ（Begin準拠）
    void SwitchShaderConfig(ID3D12GraphicsCommandList* commandList, const std::string& configName, uint32_t viewportWidth, uint32_t viewportHeight) {
        auto it = m_shaderConfigs.find(configName);
        if (it != m_shaderConfigs.end()) {
            const auto& config = it->second;

            // PSO取得（デフォルトパラメータ、必要に応じてカスタム）
            auto pso = config.pipeline->GetPipeline(
                config.renderingSetting.BlendMode, config.renderingSetting.CullMode, config.renderingSetting.PrimitiveTopologyType,
                config.renderingSetting.IsDepth, config.renderingSetting.IsDepthMask, config.renderingSetting.RTVCount, config.renderingSetting.IsWireFrame);

            if (!pso)
            {
                FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("pso is null");
                return;
            }

            commandList->SetPipelineState(pso);
            commandList->SetGraphicsRootSignature(config.rootSignature->GetRootSignature());

            // PrimitiveTopology設定（登録時のtopologyType使用）
            auto topologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(config.topologyType);
            switch (topologyType) {
            case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
                break;
            case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
                break;
            case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                break;
            case D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:
                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
                break;
            }

            // ビューポート/シザー設定（Begin準拠）
            D3D12_VIEWPORT viewport = {};
            D3D12_RECT rect = {};
            viewport.Width = static_cast<float>(viewportWidth);
            viewport.Height = static_cast<float>(viewportHeight);
            viewport.MinDepth = Def::FloatZero;
            viewport.MaxDepth = Def::FloatOne;
            rect.right = viewportWidth;
            rect.bottom = viewportHeight;
            commandList->RSSetViewports(Def::UIntOne, &viewport);
            commandList->RSSetScissorRects(Def::UIntOne, &rect);

            // BoneTransformsセットアップ（スキン用、DrawModelで使用）
            // 必要に応じてCBVバインド（例: GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(2, *config.boneTransforms);）
            // ここではセットアップのみ、実際のバインドは描画時にユーザー側で

            // ルートパラメータ再バインド（CBV/SRVなど、シグネチャ変更時必要）
        }
    }

private:
    // Shaderクラスからコピーした静的関数（独立使用のため）
    static void ReflectShaderCBuffers(ComPtr<ID3DBlob> const compiledShader, RenderingSetting& renderingSetting) noexcept {
        if (!compiledShader) return;

        ComPtr<ID3D12ShaderReflection> reflector;
        if (FAILED(D3DReflect(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), IID_PPV_ARGS(&reflector)))) {
            return;
        }

        D3D12_SHADER_DESC shaderDesc;
        if (FAILED(reflector->GetDesc(&shaderDesc))) {
            return;
        }

        // CBufferキャッシュ部分（必要ならm_cbufferCacheをクラスメンバーに、今回はスキップ）
        for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i) {
            ID3D12ShaderReflectionConstantBuffer* cb = reflector->GetConstantBufferByIndex(i);
            D3D12_SHADER_BUFFER_DESC cbDesc;
            if (FAILED(cb->GetDesc(&cbDesc))) continue;

            CBufferLayout layout;
            layout.name = cbDesc.Name;
            layout.size = cbDesc.Size;

            for (UINT v = 0; v < cbDesc.Variables; ++v) {
                ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
                D3D12_SHADER_VARIABLE_DESC varDesc;
                if (FAILED(var->GetDesc(&varDesc))) continue;

                ShaderVariableInfo varInfo;
                varInfo.name = varDesc.Name;
                varInfo.startOffset = varDesc.StartOffset;
                varInfo.size = varDesc.Size;
                layout.variables.push_back(varInfo);
            }
            // m_cbufferCache[layout.name] = layout;  // 必要なら追加
        }

        // 入力レイアウト反映
        for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
            D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
            if (FAILED(reflector->GetInputParameterDesc(i, &paramDesc))) continue;

            if (strcmp(paramDesc.SemanticName, "TEXCOORD") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::TEXCOORD);
            }
            else if (strcmp(paramDesc.SemanticName, "POSITION") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::POSITION);
            }
            else if (strcmp(paramDesc.SemanticName, "NORMAL") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::NORMAL);
            }
            else if (strcmp(paramDesc.SemanticName, "TANGENT") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::TANGENT);
            }
            else if (strcmp(paramDesc.SemanticName, "COLOR") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::COLOR);
            }
            else if (strcmp(paramDesc.SemanticName, "SKININDEX") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::SKININDEX);
            }
            else if (strcmp(paramDesc.SemanticName, "SKINWEIGHT") == 0 && paramDesc.SemanticIndex == 0) {
                renderingSetting.InputLayouts.push_back(InputLayout::SKINWEIGHT);
            }
        }
    }

    static void ShaderView(ComPtr<ID3DBlob> const compiledShader, std::vector<RangeType>& rangeTypes) noexcept {
        if (!compiledShader) return;

        ComPtr<ID3D12ShaderReflection> reflector;
        if (FAILED(D3DReflect(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), IID_PPV_ARGS(&reflector)))) return;

        D3D12_SHADER_DESC shaderDesc;
        reflector->GetDesc(&shaderDesc);

        for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            reflector->GetResourceBindingDesc(i, &bindDesc);

            switch (bindDesc.Type) {
            case D3D_SIT_CBUFFER:
                rangeTypes.push_back(RangeType::CBV);
                break;
            case D3D_SIT_TEXTURE:
                rangeTypes.push_back(RangeType::SRV);
                break;
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
                rangeTypes.push_back(RangeType::UAV);
                break;
            default:
                break;
            }
        }

        std::sort(rangeTypes.begin(), rangeTypes.end(), [](RangeType a, RangeType b) noexcept {
            return static_cast<int>(a) < static_cast<int>(b);
            });
    }

    struct ShaderConfig {
        ComPtr<ID3DBlob> vsBlob, psBlob, hsBlob, dsBlob, gsBlob;
        std::unique_ptr<RootSignature> rootSignature;
        std::unique_ptr<Pipeline> pipeline;
        std::unique_ptr<CBufferData::BoneTransforms> boneTransforms;
        RenderingSetting renderingSetting;
        PrimitiveTopologyType topologyType{};  // 保存用
    };

    GraphicsDevice* m_device;
    FlResourceAdministrator* m_blobManager;  // 既存Blobマネージャー
    std::unordered_map<std::string, ShaderConfig> m_shaderConfigs;
};