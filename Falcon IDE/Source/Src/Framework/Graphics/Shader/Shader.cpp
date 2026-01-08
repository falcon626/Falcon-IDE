#include "Shader.h"

const bool Shader::Initializer(GraphicsDevice* pGraphicsDevice, const uint32_t windowWidth, const uint32_t windowHeight)
{
	if (!pGraphicsDevice) return false;

	m_pDevice      = pGraphicsDevice;

	m_windowWidth  = windowWidth;
	m_windowHeight = windowHeight;

	m_upRootSignature = std::make_unique<RootSignature>();
	m_upPipeline = std::make_unique<Pipeline>();

	return true;
}

void Shader::Create()
{
	m_cbvCount = Def::UIntZero;
	m_rangeTypes.clear();
	m_renderingSetting.Reset();

	m_renderingSetting.Formats = { DXGI_FORMAT_R8G8B8A8_UNORM };

	ReflectShaderCBuffers(m_pVSBlob, m_renderingSetting);

	auto pBlobs{ std::vector<ComPtr<ID3DBlob>>{m_pVSBlob ,m_pHSBlob ,m_pDSBlob ,m_pGSBlob ,m_pPSBlob} };

	for (auto&& pBlob : pBlobs)
		ShaderView(pBlob, m_rangeTypes);

	m_upRootSignature->Create(m_pDevice, m_rangeTypes, m_cbvCount);

	m_upPipeline->SetRenderSettings(m_pDevice, m_upRootSignature.get(), m_renderingSetting.InputLayouts,
		m_renderingSetting.CullMode, m_renderingSetting.BlendMode, m_renderingSetting.PrimitiveTopologyType);
	m_upPipeline->Create(pBlobs, m_renderingSetting.Formats,
		m_renderingSetting.IsDepth, m_renderingSetting.IsDepthMask, m_renderingSetting.RTVCount, m_renderingSetting.IsWireFrame);
}

void Shader::Begin()
{
	m_pDevice->GetCmdList()->SetPipelineState(m_upPipeline->GetPipeline(
		BlendMode::Opaque, CullMode::Front, PrimitiveTopologyType::Triangle, 
		true, true, Def::IntOne, false));

	// ルートシグネチャのセット
	m_pDevice->GetCmdList()->SetGraphicsRootSignature(m_upRootSignature->GetRootSignature());

	auto topologyType{ static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(m_upPipeline->GetTopologyType()) };

	switch (topologyType)
	{
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
		m_pDevice->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
		m_pDevice->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
		m_pDevice->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:
		m_pDevice->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
		break;
	}

	auto viewport{ D3D12_VIEWPORT{} };
	auto rect    { D3D12_RECT{} };

	viewport.Width    = static_cast<float>(m_windowWidth);
	viewport.Height   = static_cast<float>(m_windowHeight);
	viewport.MinDepth = Def::FloatZero;
	viewport.MaxDepth = Def::FloatOne;

	rect.right  = m_windowWidth;
	rect.bottom = m_windowHeight;

	m_pDevice->GetCmdList()->RSSetViewports(Def::UIntOne, &viewport);
	m_pDevice->GetCmdList()->RSSetScissorRects(Def::UIntOne, &rect);

	m_upWorldMatrix    = std::make_unique<CBufferData::WorldMatrix>();
	m_upBoneTransforms = std::make_unique<CBufferData::BoneTransforms>();
	m_upIsSkinMesh	   = std::make_unique<CBufferData::IsSkinMesh>();
	m_upMaterialCorlor = std::make_unique < CBufferData::MaterialCBData> ();
}

void Shader::Begin(ComPtr<ID3D12GraphicsCommandList6>& cmdList)
{
	cmdList->SetPipelineState(m_upPipeline->GetPipeline(
		BlendMode::Opaque, CullMode::Back, PrimitiveTopologyType::Triangle,
		true, true, Def::IntOne, false));

	// ルートシグネチャのセット
	cmdList->SetGraphicsRootSignature(m_upRootSignature->GetRootSignature());

	auto topologyType{ static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(m_upPipeline->GetTopologyType()) };

	switch (topologyType)
	{
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
		break;
	}

	auto viewport{ D3D12_VIEWPORT{} };
	auto rect{ D3D12_RECT{} };

	viewport.Width = static_cast<float>(m_windowWidth);
	viewport.Height = static_cast<float>(m_windowHeight);
	viewport.MinDepth = Def::FloatZero;
	viewport.MaxDepth = Def::FloatOne;

	rect.right = m_windowWidth;
	rect.bottom = m_windowHeight;

	cmdList->RSSetViewports(Def::UIntOne, &viewport);
	cmdList->RSSetScissorRects(Def::UIntOne, &rect);

	m_upWorldMatrix	   = std::make_unique<CBufferData::WorldMatrix>();
	m_upBoneTransforms = std::make_unique<CBufferData::BoneTransforms>();
	m_upIsSkinMesh	   = std::make_unique<CBufferData::IsSkinMesh>();
	m_upMaterialCorlor = std::make_unique<CBufferData::MaterialCBData>();
}

void Shader::DrawMesh(const Mesh& mesh)
{
	SetMaterial(mesh.GetMaterial());

	mesh.DrawInstanced(mesh.GetInstanceCount());
}

void CalculateNodeWorldMatricesRecursive(
	const std::vector<ModelData::Node>& nodes,
	int nodeIndex,
	const Math::Matrix& parentWorld,
	std::vector<Math::Matrix>& outWorldMatrices)
{
	const auto& node{ nodes[nodeIndex] };
	auto world{ Def::Mat };
	world = node.m_mLocal * parentWorld;
	outWorldMatrices[nodeIndex] = world;

	for (int childIdx : node.m_children) {
		CalculateNodeWorldMatricesRecursive(nodes, childIdx, world, outWorldMatrices);
	}
}

void CalculateNodeWorldMatrices(
	const std::vector<ModelData::Node>& nodes,
	const size_t nodeSize,
	std::vector<Math::Matrix>& outWorldMatrices)
{
	outWorldMatrices.resize(nodes.size());
	for (auto idx{ Def::UIntZero }; idx < nodeSize ; ++idx) {
		if (nodes[idx].m_parentIndex == -Def::IntOne) 
		{
			auto world{ nodes[idx].m_mLocal };
			CalculateNodeWorldMatricesRecursive(nodes, idx, world, outWorldMatrices);
		}
	}
}

void Shader::DrawModel(ModelData& modelData, const Math::Matrix& worldMatrix) 
{
	if (!modelData.IsSkinMesh()) 
	{
		// 通常の描画
		for (const auto& node : modelData.GetNodes()) {
			auto world{ node.m_mLocal * worldMatrix };

			m_upIsSkinMesh->isSkin = FALSE;
			GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(3, *m_upIsSkinMesh);
			m_upWorldMatrix->world = world;
			GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(1, *m_upWorldMatrix);
			if (node.m_spMesh) DrawMesh(*node.m_spMesh);
		}
		return;
	}

	auto mats{ std::vector<Math::Matrix>{} };
	CalculateNodeWorldMatrices(modelData.WorkNodes(), modelData.GetNodes().size(), mats);

	for (const auto& node : modelData.WorkNodes()) {
		if (node.m_boneIndex != -Def::IntOne && node.m_boneIndex < m_upBoneTransforms->boneTransforms.size()) 
		{
			m_upBoneTransforms->boneTransforms[node.m_boneIndex] = node.m_mBoneInverseWorld *
				mats[modelData.WorkBoneNodeIndices()[node.m_boneIndex]];
		}
	}
	m_upIsSkinMesh->isSkin = TRUE;
	GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(3, *m_upIsSkinMesh);
	GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(2, *m_upBoneTransforms);

	auto nodes{ modelData.GetNodes() };
	// メッシュ描画
	for (const auto& meshIdx : modelData.GetMeshNodeIndices()) {
		auto world{ mats[meshIdx] * worldMatrix };

		m_upWorldMatrix->world = world;
		GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(1,*m_upWorldMatrix);

		DrawMesh(*nodes[meshIdx].m_spMesh);
	}
}

void Shader::DrawModel(ModelData& modelData, const Math::Matrix& worldMatrix, ComPtr<ID3D12GraphicsCommandList6>& cmdList)
{
	// 通常の描画
	for (const auto& node : modelData.GetNodes()) {
		auto world{ node.m_mLocal * worldMatrix };

		GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(1, world, cmdList);
		if (node.m_spMesh) DrawMesh(*node.m_spMesh);
	}
}

void Shader::ReflectShaderCBuffers(ComPtr<ID3DBlob> const compiledShader, RenderingSetting& renderingSetting) noexcept
{
	if (!compiledShader)return;

	auto reflector{ ComPtr<ID3D12ShaderReflection>{} };
	if (FAILED(D3DReflect(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(),
		IID_PPV_ARGS(&reflector)))) {
		return;
	}

	auto shaderDesc{ D3D12_SHADER_DESC{} };
	if (FAILED(reflector->GetDesc(&shaderDesc))) {
		return;
	}

	for (auto i{ Def::UIntZero }; i < shaderDesc.ConstantBuffers; ++i) {
		auto cb{ reflector->GetConstantBufferByIndex(i) };

		D3D12_SHADER_BUFFER_DESC cbDesc;
		if (FAILED(cb->GetDesc(&cbDesc))) {
			continue;
		}

		auto layout{ CBufferLayout{} };
		layout.name = cbDesc.Name;
		layout.size = cbDesc.Size;

		for (auto v{ Def::UIntZero }; v < cbDesc.Variables; ++v) {
			auto var{ cb->GetVariableByIndex(v) };

			auto varDesc{ D3D12_SHADER_VARIABLE_DESC {} };
			if (FAILED(var->GetDesc(&varDesc))) continue;

			ShaderVariableInfo varInfo;
			varInfo.name = varDesc.Name;
			varInfo.startOffset = varDesc.StartOffset;
			varInfo.size = varDesc.Size;

			layout.variables.push_back(varInfo);
		}
		m_cbufferCache[layout.name] = layout;
	}

	for (auto i{ Def::UIntZero }; i < shaderDesc.InputParameters; ++i) {
		auto paramDesc{ D3D12_SIGNATURE_PARAMETER_DESC{} };

		if (FAILED(reflector->GetInputParameterDesc(i, &paramDesc))) continue;

		if (!lstrcmpA(paramDesc.SemanticName, "TEXCOORD")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::TEXCOORD);
			}
		}
		else if (!lstrcmpA(paramDesc.SemanticName, "POSITION")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::POSITION);
			}
		}
		else if (!lstrcmpA(paramDesc.SemanticName, "NORMAL")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::NORMAL);
			}
		}
		else if (!lstrcmpA(paramDesc.SemanticName, "TANGENT")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::TANGENT);
			}
		}
		else if (!lstrcmpA(paramDesc.SemanticName, "COLOR")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::COLOR);
			}
		}
		else if (!lstrcmpA(paramDesc.SemanticName, "SKININDEX")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::SKININDEX);
			}
		}
		else if (!lstrcmpA(paramDesc.SemanticName, "SKINWEIGHT")) {
			if (paramDesc.SemanticIndex == Def::UIntZero) {
				renderingSetting.InputLayouts.push_back(InputLayout::SKINWEIGHT);
			}
		}
	}
}

void Shader::ShaderView(ComPtr<ID3DBlob> const compiledShader, std::vector<RangeType>& rangeTypes) noexcept
{
	if (!compiledShader)return;

	auto reflector{ ComPtr<ID3D12ShaderReflection>{} };
	if (FAILED(D3DReflect(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(),
		IID_PPV_ARGS(&reflector)))) return;

	auto shaderDesc{ D3D12_SHADER_DESC{} };
	reflector->GetDesc(&shaderDesc);

	for (auto i{ Def::UIntZero }; i < shaderDesc.BoundResources; ++i)
	{
		auto bindDesc{ D3D12_SHADER_INPUT_BIND_DESC{} };
		reflector->GetResourceBindingDesc(i, &bindDesc);

		switch (bindDesc.Type)
		{
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

	std::sort(rangeTypes.begin(), rangeTypes.end(),
		[](RangeType a, RangeType b) noexcept
		{
			return static_cast<int32_t>(a) < static_cast<int32_t>(b);
		});
}

void Shader::SetMaterial(const Material& material)
{
	m_upMaterialCorlor->baseColor = material.BaseColor;

	GraphicsDevice::Instance().GetCBufferAllocater()
		->BindAndAttachData(4, *m_upMaterialCorlor);

	if (material.spBaseColorTex != nullptr) material.spBaseColorTex->Set(m_cbvCount);
	//material.spNormalTex->Set(m_cbvCount + 1);
	//material.spMetallicRoughnessTex->Set(m_cbvCount + 2);
	//material.spEmissiveTex->Set(m_cbvCount + 3);
}