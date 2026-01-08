#include "Pipeline.h"

#include "../RootSignature/RootSignature.h"

void Pipeline::SetRenderSettings(GraphicsDevice* pGraphicsDevice, RootSignature* pRootSignature,
    const std::vector<InputLayout>& inputLayouts, CullMode cullMode,
    BlendMode blendMode, PrimitiveTopologyType topologyType, bool isWireFrame)
{
    m_pDevice = pGraphicsDevice;
    m_pRootSignature = pRootSignature;
    m_inputLayouts = inputLayouts;
    m_cullMode = cullMode;
    m_blendMode = blendMode;
    m_topologyType = topologyType;
    m_isWireFrame = isWireFrame;
}

std::string Pipeline::GeneratePSOKey(BlendMode blendMode, CullMode cullMode, PrimitiveTopologyType topologyType,
    bool isDepth, bool isDepthMask, int rtvCount, bool isWireFrame) const
{
    return std::to_string(static_cast<int>(blendMode)) + "_" +
        std::to_string(static_cast<int>(cullMode)) + "_" +
        std::to_string(static_cast<int>(topologyType)) + "_" +
        (isDepth ? "1" : "0") + "_" +
        (isDepthMask ? "1" : "0") + "_" +
        std::to_string(rtvCount) + "_" +
        (isWireFrame ? "1" : "0");
}

void Pipeline::Create(std::vector<ComPtr<ID3DBlob>> pBlobs, const std::vector<DXGI_FORMAT> formats,
    bool isDepth, bool isDepthMask, int rtvCount, bool isWireFrame)
{
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts;
    SetInputLayout(inputLayouts, m_inputLayouts);

    m_rtvFormats = formats;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineState = {};

    // シェーダー設定
    graphicsPipelineState.VS.pShaderBytecode = pBlobs[0]->GetBufferPointer();
    graphicsPipelineState.VS.BytecodeLength = pBlobs[0]->GetBufferSize();
	m_pVs = pBlobs[0];

    if (pBlobs[1]) 
    {
        graphicsPipelineState.HS.pShaderBytecode = pBlobs[1]->GetBufferPointer();
        graphicsPipelineState.HS.BytecodeLength = pBlobs[1]->GetBufferSize();
        m_pHs = pBlobs[1];
    }
    if (pBlobs[2]) 
    {
        graphicsPipelineState.DS.pShaderBytecode = pBlobs[2]->GetBufferPointer();
        graphicsPipelineState.DS.BytecodeLength = pBlobs[2]->GetBufferSize();
        m_pDs = pBlobs[2];
    }
    if (pBlobs[3]) 
    {
        graphicsPipelineState.GS.pShaderBytecode = pBlobs[3]->GetBufferPointer();
        graphicsPipelineState.GS.BytecodeLength = pBlobs[3]->GetBufferSize();
        m_pGs = pBlobs[3];
    }
    graphicsPipelineState.PS.pShaderBytecode = pBlobs[4]->GetBufferPointer();
    graphicsPipelineState.PS.BytecodeLength = pBlobs[4]->GetBufferSize();
    m_pPs = pBlobs[4];

    graphicsPipelineState.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // カリングモード
    graphicsPipelineState.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(m_cullMode);
    graphicsPipelineState.RasterizerState.FillMode = isWireFrame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

    // 深度設定
    if (isDepth) {
        graphicsPipelineState.RasterizerState.DepthClipEnable = true;
        graphicsPipelineState.DepthStencilState.DepthEnable = true;
        graphicsPipelineState.DepthStencilState.DepthWriteMask = isDepthMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        graphicsPipelineState.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        graphicsPipelineState.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }
    else {
        graphicsPipelineState.RasterizerState.DepthClipEnable = false;
        graphicsPipelineState.DepthStencilState.DepthEnable = false;
        graphicsPipelineState.DSVFormat = DXGI_FORMAT_UNKNOWN;
    }

    graphicsPipelineState.BlendState.AlphaToCoverageEnable = false;
    graphicsPipelineState.BlendState.IndependentBlendEnable = false;

    // ブレンド設定
    D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
    SetBlendMode(blendDesc, m_blendMode);
    graphicsPipelineState.BlendState.RenderTarget[0] = blendDesc;

    // レイアウト設定
    graphicsPipelineState.InputLayout.pInputElementDescs = inputLayouts.data();
    graphicsPipelineState.InputLayout.NumElements = static_cast<int>(m_inputLayouts.size());

    // トポロジー
    graphicsPipelineState.PrimitiveTopologyType = (pBlobs[3] && pBlobs[4]) ?
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH : static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(m_topologyType);

    graphicsPipelineState.NumRenderTargets = rtvCount;
    for (int i = 0; i < rtvCount; ++i) {
        graphicsPipelineState.RTVFormats[i] = formats[i];
    }

    graphicsPipelineState.SampleDesc.Count = Def::UIntOne;
    graphicsPipelineState.pRootSignature = m_pRootSignature->GetRootSignature();

    // PSOをキャッシュに保存
    std::string key = GeneratePSOKey(m_blendMode, m_cullMode, m_topologyType, isDepth, isDepthMask, rtvCount, isWireFrame);
    ComPtr<ID3D12PipelineState> pipelineState;
    HRESULT hr = m_pDevice->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineState, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) 
    {
        assert(false && "パイプラインステートの作成に失敗しました");
        return;
    }

    auto pipeName{ L"PipelineState" + ansi_to_wide(key) };
    pipelineState->SetName(pipeName.c_str());

    m_psoCache[key] = pipelineState;
}

ID3D12PipelineState* Pipeline::GetPipeline(BlendMode blendMode, CullMode cullMode, PrimitiveTopologyType topologyType,
    bool isDepth, bool isDepthMask, int rtvCount, bool isWireFrame)
{
    auto key{ GeneratePSOKey(blendMode, cullMode, topologyType, isDepth, isDepthMask, rtvCount, isWireFrame) };
    auto it { m_psoCache.find(key) };
    if (it != m_psoCache.end()) return it->second.Get(); // キャッシュからPSOを取得

    // キャッシュにない場合は新しいPSOを作成
    auto prevBlendMode   { m_blendMode };
    auto prevCullMode    { m_cullMode };
    auto prevTopologyType{ m_topologyType };
    auto prevWireFrame   { m_isWireFrame };


    m_blendMode    = blendMode;
    m_cullMode     = cullMode;
    m_topologyType = topologyType;
    m_isWireFrame  = isWireFrame;

    Create({ m_pVs, m_pHs, m_pDs, m_pGs, m_pPs }, 
        m_rtvFormats, isDepth, isDepthMask, rtvCount, isWireFrame);

    m_blendMode    = prevBlendMode;
    m_cullMode     = prevCullMode;
    //m_topologyType = prevTopologyType;
    m_isWireFrame  = prevWireFrame;

    return m_psoCache[key].Get();
}

void Pipeline::SetBlendMode(D3D12_RENDER_TARGET_BLEND_DESC& blendDesc, BlendMode blendMode)
{
    blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    switch (blendMode) {
    case BlendMode::Opaque:
        blendDesc.BlendEnable = false; // ブレンド無効
        break;
    case BlendMode::Add:
        blendDesc.BlendEnable    = true;
        blendDesc.BlendOp        = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlend       = D3D12_BLEND_SRC_ALPHA;
        blendDesc.DestBlend      = D3D12_BLEND_ONE;
        blendDesc.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlendAlpha  = D3D12_BLEND_ONE;
        blendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.LogicOp        = D3D12_LOGIC_OP_NOOP;
        break;
    case BlendMode::Alpha:
        blendDesc.BlendEnable    = true;
        blendDesc.BlendOp        = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlend       = D3D12_BLEND_SRC_ALPHA;
        blendDesc.DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlendAlpha  = D3D12_BLEND_ONE;
        blendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.LogicOp        = D3D12_LOGIC_OP_NOOP;
        break;
    }
}

void Pipeline::SetInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements, const std::vector<InputLayout>& inputLayouts)
{
	auto slot{ Def::UIntZero };
	for (auto i{ Def::IntZero }; i < static_cast<int32_t>(inputLayouts.size()); ++i)
	{
		if (inputLayouts[i] == InputLayout::POSITION)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, slot++,
				D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}
		else if (inputLayouts[i] == InputLayout::TEXCOORD)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,slot++,
				D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ,0 });
		}
		else if (inputLayouts[i] == InputLayout::NORMAL)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, slot++,
				D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}
		else if (inputLayouts[i] == InputLayout::COLOR)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, slot++,
				D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}
		else if (inputLayouts[i] == InputLayout::TANGENT)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, slot++,
				D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}
		else if (inputLayouts[i] == InputLayout::SKININDEX)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "SKININDEX", 0, DXGI_FORMAT_R16G16B16A16_UINT, slot++,
				D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}
		else if (inputLayouts[i] == InputLayout::SKINWEIGHT)
		{
			inputElements.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "SKINWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, slot++,
				D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}
	}
}