#include "Mesh.h"

void Mesh::Create(GraphicsDevice* pGraphicsDevice, const MeshVertex& vertices,
	const std::vector<MeshFace>& faces, const Material& material, const size_t vertexCount)
{
	m_pDevice  = pGraphicsDevice;
	m_material = material;
	
	for (auto&& layout : m_semanticsLayout)
	{
		switch (layout)
		{
		case InputLayout::POSITION:
			CreateVertexBuffer(vertices.Position, sizeof(Math::Vector3),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.Position)), &m_pPosBuffer, m_posView, m_views, vertexCount);
			break;
		case InputLayout::TEXCOORD:
			CreateVertexBuffer(vertices.UV, sizeof(Math::Vector2),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.UV)), &m_pUVBuffer, m_UVView, m_views, vertexCount);
			break;
		case InputLayout::NORMAL:
			CreateVertexBuffer(vertices.Normal, sizeof(Math::Vector3),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.Normal)), &m_pNorBuffer, m_norView, m_views, vertexCount);
			break;
		case InputLayout::TANGENT:
			CreateVertexBuffer(vertices.Tangent, sizeof(Math::Vector3),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.Tangent)), &m_pTanBuffer, m_tanView, m_views, vertexCount);
			break;
		case InputLayout::COLOR:
			CreateVertexBuffer(vertices.Color, sizeof(uint32_t),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.Color)), &m_pColBuffer, m_colView, m_views, vertexCount);
			break;
		case InputLayout::SKININDEX:
			CreateVertexBuffer(vertices.SkinIndexList, sizeof(std::array<short, 4>),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.SkinIndexList)), &m_pSkiIBuffer, m_skiIView, m_views, vertexCount);
			break;
		case InputLayout::SKINWEIGHT:
			CreateVertexBuffer(vertices.SkinWeightList, sizeof(std::array<float, 4>),
				ansi_to_wide(_CRT_STRINGIZE_(vertices.SkinWeightList)), &m_pSkiWBuffer, m_skiWView, m_views, vertexCount);
			break;
		default: break;
		}
	}

	m_instanceCount = static_cast<UINT>(faces.size() * 3);

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	resDesc.Width = sizeof(MeshFace) * faces.size();

	// インデックスバッファ作成
	auto hr = m_pDevice->GetDevice()->CreateCommittedResource(&heapProp,
		D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
		IID_PPV_ARGS(&m_pIBuffer));

	if (FAILED(hr))
	{
		assert(false && "インデックスバッファー作成失敗");
		return;
	}

	// インデックスバッファのデータをビューに書き込む
	m_ibView.BufferLocation = m_pIBuffer->GetGPUVirtualAddress();
	m_ibView.SizeInBytes = static_cast<UINT>(resDesc.Width);
	m_ibView.Format = DXGI_FORMAT_R32_UINT;

	// インデックスバッファに情報を書き込む
	MeshFace* ibMap = nullptr;
	{
		m_pIBuffer->Map(0, nullptr, reinterpret_cast<void**>(&ibMap));
		std::copy(std::begin(faces), std::end(faces), ibMap);
		m_pIBuffer->Unmap(0, nullptr);
	}
}

void Mesh::DrawInstanced(UINT vertexCount)const
{
	m_pDevice->GetCmdList()->IASetVertexBuffers(0, static_cast<UINT>(m_views.size()), m_views.data());

	m_pDevice->GetCmdList()->IASetIndexBuffer(&m_ibView);

	m_pDevice->GetCmdList()->DrawIndexedInstanced(vertexCount, 1, 0, 0, 0);
}