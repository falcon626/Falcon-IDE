//#include "KdMesh.h"
//
//#if true
////=============================================================
////
//// Mesh
////
////=============================================================
//
//bool KdMesh::Create(const std::string& parentDirPath, const std::vector<KdMeshScene::MeshData>& meshDatas)
//{
//	Release();
//
////	m_srcMeshDatas = meshDatas;
//
//	//KdMeshScene::VertexData				vertexData;
//	m_vertexData.Clear();
//	//std::vector<KdMeshScene::FaceData>	faceDatas;
//	m_faceDatas.clear();
//
//	//------------------------------
//	// マテリアル単位で分割されているメッシュを
//	// １つにまとめる
//	//------------------------------
//	{
//		m_subsets.clear();
//		uint32_t faceStart = 0;
//		uint32_t vertexStart = 0;
//
//		for (size_t im = 0; im < meshDatas.size(); im++)
//		{
//			//----------------------
//			// サブセット情報の作成
//			//----------------------
//			KdMeshSubset subset;
//
//			subset.MaterialNo = meshDatas[im].MaterialIndex;
//
//			subset.FaceStart = faceStart;
//			subset.FaceCount = (uint32_t)meshDatas[im].FaceDatas.size();
//
//			m_subsets.push_back(subset);
//
//			//----------------------
//			// 頂点情報を合成
//			//----------------------
//			m_vertexData.Combine(meshDatas[im].Vertex);
//
//			//----------------------
//			// 面情報の合成
//			//----------------------
//			for (auto face : meshDatas[im].FaceDatas)
//			{
//				// 頂点Index調整
//				face.Idx[0] += vertexStart;
//				face.Idx[1] += vertexStart;
//				face.Idx[2] += vertexStart;
//
//				m_faceDatas.push_back(face);
//			}
//
//			// 追加された頂点数ぶん、開始Indexをずらす
//			vertexStart += (uint32_t)meshDatas[im].Vertex.PosList.size();
//			faceStart += subset.FaceCount;
//		}
//	}
//
//	//------------------------------
//	//------------------------------
//
//	//------------------------------
//	// 頂点バッファ(座標)作成
//	//------------------------------
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_vertexData.PosList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_Pos = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_Pos->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(KdVector3) * (uint32_t)m_vertexData.PosList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//
//		// AA境界データ作成
//		DirectX::BoundingBox::CreateFromPoints(m_aabb, m_vertexData.PosList.size(), &m_vertexData.PosList[0], sizeof(KdVector3));
//		// 境界球データ作成
//		DirectX::BoundingSphere::CreateFromPoints(m_bs, m_vertexData.PosList.size(), &m_vertexData.PosList[0], sizeof(KdVector3));
//
//		m_stride.push_back(sizeof(KdVector3));
//		m_offset.push_back(0);
//
//		m_vertexCount = (uint32_t)m_vertexData.PosList.size();
//	}
//	//------------------------------
//	// 頂点バッファ(UV)作成
//	//------------------------------
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//
//		// UVが空ならダミーデータを入れる
//		if (m_vertexData.UVList.empty())
//		{
//			m_vertexData.UVList.resize(m_vertexData.PosList.size());
//			for (size_t i = 0; i < m_vertexData.PosList.size(); i++)
//			{
//				m_vertexData.UVList[i] = { 0.0f, 0.0f };
//			}
//		}
//
//		initData.pSysMem = &m_vertexData.UVList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_UV = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_UV->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(KdVector2) * (uint32_t)m_vertexData.UVList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//	}
//
//	m_stride.push_back(sizeof(KdVector2));
//	m_offset.push_back(0);
//
//	//------------------------------
//	// 頂点バッファ(Tangent)作成
//	//------------------------------
//	if(m_vertexData.TangentList.size() >= 1)
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_vertexData.TangentList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_Tangent = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_Tangent->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(KdVector3) * (uint32_t)m_vertexData.TangentList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//
//	}
//
//	m_stride.push_back(sizeof(KdVector3));
//	m_offset.push_back(0);
//
//	//------------------------------
//	// 頂点バッファ(Normal)作成
//	//------------------------------
//	if (m_vertexData.NormalList.size() >= 1)
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_vertexData.NormalList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_Normal = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_Normal->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(KdVector3) * (uint32_t)m_vertexData.NormalList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//
//	}
//
//	m_stride.push_back(sizeof(KdVector3));
//	m_offset.push_back(0);
//
//	//------------------------------
//	// 頂点バッファ(Color)作成
//	//------------------------------
//	if (m_vertexData.ColorList.size() >= 1)
//	{
//		std::vector<UINT> tmpBuffer;
//
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_vertexData.ColorList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_Color = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_Color->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(m_vertexData.ColorList[0]) * (uint32_t)m_vertexData.ColorList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//
//	}
//
//	m_stride.push_back(sizeof(m_vertexData.ColorList[0]));
//	m_offset.push_back(0);
//
//	//------------------------------
//	// 頂点バッファ(SkinIndexList)作成
//	//------------------------------
//	if (m_vertexData.SkinIndexList.size() >= 1)
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_vertexData.SkinIndexList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_SkinIndex = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_SkinIndex->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(m_vertexData.SkinIndexList[0]) * (uint32_t)m_vertexData.SkinIndexList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//	}
//
//	m_stride.push_back(sizeof(m_vertexData.SkinIndexList[0]));
//	m_offset.push_back(0);
//
//	//------------------------------
//	// 頂点バッファ(SkinWeightList)作成
//	//------------------------------
//	if (m_vertexData.SkinWeightList.size() >= 1)
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_vertexData.SkinWeightList[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_vb_SkinWeight = std::make_shared<KdBuffer>();
//		if (FAILED(m_vb_SkinWeight->Create(D3D11_BIND_VERTEX_BUFFER, sizeof(m_vertexData.SkinWeightList[0]) * (uint32_t)m_vertexData.SkinWeightList.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//	}
//
//	// １頂点サイズ情報
//	m_stride.push_back(sizeof(m_vertexData.SkinWeightList[0]));
//	m_offset.push_back(0);
//
//	//------------------------------
//	// スキンメッシュ専用
//	//------------------------------
//	/*
//	if (m_vb_SkinIndex != nullptr)
//	{
//		m_vs_SkinnedPos = std::make_shared<KdBuffer>();
//		m_vs_SkinnedPos->CreateStreamOutputBuffer(m_vb_Pos->GetBufferSize());
//
//		m_vs_SkinnedTangent = std::make_shared<KdBuffer>();
//		m_vs_SkinnedTangent->CreateStreamOutputBuffer(m_vb_Tangent->GetBufferSize());
//
//		m_vs_SkinnedNormal = std::make_shared<KdBuffer>();
//		m_vs_SkinnedNormal->CreateStreamOutputBuffer(m_vb_Normal->GetBufferSize());
//	}
//	*/
//
//	//------------------------------
//	// インデックスバッファ作成
//	//------------------------------
//	{
//		// 書き込むデータ
//		D3D11_SUBRESOURCE_DATA initData;
//		initData.pSysMem = &m_faceDatas[0];				// バッファに書き込む頂点配列の先頭アドレス
//		initData.SysMemPitch = 0;
//		initData.SysMemSlicePitch = 0;
//
//		// バッファ作成
//		m_ib = std::make_shared<KdBuffer>();
//		if (FAILED(m_ib->Create(D3D11_BIND_INDEX_BUFFER, sizeof(KdMeshScene::FaceData) * (uint32_t)m_faceDatas.size(), D3D11_USAGE_DEFAULT, 0, &initData)))
//		{
//			Release();
//			return false;
//		}
//	}
//
//	/*
//	//------------------------------
//	// マテリアル
//	//------------------------------
//
//	// シェーダー
//	auto shader = KdShaderManager::GetInstance().GetShader("Lit");
//
//
//
//	// マテリアル作成
//	m_material = shader->CreateMaterial();
//
//	meshDatas[0].materialIndex;
//	m_material->SetData(meshData.material.Name);
//
//	// 基本色
//	m_material->SetValue(0, "g_BaseColor", meshData.material.BaseColor);
//	// メタリック
//	m_material->SetValue(0, "g_Metallic", meshData.material.Metallic);
//	// スムーズネス
//	m_material->SetValue(0, "g_Smoothness", meshData.material.Smoothness);
//	// 放射色
//	m_material->SetValue(0, "g_EmissiveColor", meshData.material.EmissiveColor);
//
//	// 0番目のパス固定で使用
//	auto shaderPass = shader->GetPass(0);
//
//	// テクスチャ
//	for (auto&& info : shaderPass->GetResouceInputInfos())
//	{
//		// 基本テクスチャ
//		if (info.second.m_name == "g_baseMap")
//		{
//			auto tex = KdCoreSystem::GetInstance().m_loadTextureProc(parentDirPath + "\\" + meshData.material.BaseMapFilename);
//			if(tex->IsValid())
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, tex);
//			}
//			else
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, D3D.GetDotWhiteTex());
//			}
//		}
//		else if (info.second.m_name == "g_normalMap")
//		{
//			auto tex = KdCoreSystem::GetInstance().m_loadTextureProc(parentDirPath + "\\" + meshData.material.NormalMapFilename);
//			if (tex->IsValid())
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, tex);
//			}
//			else
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, D3D.GetDotNormalTex());
//			}
//		}
//		else if (info.second.m_name == "g_metallicSmoothnesMap")
//		{
//			std::filesystem::path baseTexPath = meshData.material.BaseMapFilename;
//			std::string smMapFilename = baseTexPath.stem().generic_string() + ".ms" + baseTexPath.extension().generic_string();
//
//
//			auto tex = KdCoreSystem::GetInstance().m_loadTextureProc(parentDirPath + "\\" + smMapFilename);
//			if (tex->IsValid())
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, tex);
//			}
//			else
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, D3D.GetDotWhiteTex());
//			}
//		}
//		else if (info.second.m_name == "g_emissiveMap")
//		{
//			auto tex = KdCoreSystem::GetInstance().m_loadTextureProc(parentDirPath + "\\" + meshData.material.EmissiveMapFilename);
//			if (tex->IsValid())
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, tex);
//			}
//			else
//			{
//				m_material->SetTexture(0, info.second.m_bindIdx, D3D.GetDotWhiteTex());
//			}
//		}
//
//	}
//	*/
//
//	return true;
//}
//
//std::shared_ptr<KdMeshGPUSkinningData> KdMesh::CreateGPUSkinningData()
//{
//	auto data = std::make_shared<KdMeshGPUSkinningData>();
//
//	if (m_vb_SkinIndex != nullptr)
//	{
//		data->m_vs_SkinnedPos = std::make_shared<KdBuffer>();
//		data->m_vs_SkinnedPos->CreateStreamOutputBuffer(m_vb_Pos->GetBufferSize());
//
//		data->m_vs_SkinnedTangent = std::make_shared<KdBuffer>();
//		data->m_vs_SkinnedTangent->CreateStreamOutputBuffer(m_vb_Tangent->GetBufferSize());
//
//		data->m_vs_SkinnedNormal = std::make_shared<KdBuffer>();
//		data->m_vs_SkinnedNormal->CreateStreamOutputBuffer(m_vb_Normal->GetBufferSize());
//	}
//
//	return data;
//}
//
//
//void KdMesh::SetToDevice(const KdMeshGPUSkinningData* skinnedData) const
//{
//
//	std::vector<ID3D11Buffer*>	buffers;
//
//	// 0 : 座標
//	if (skinnedData == nullptr || skinnedData->m_vs_SkinnedPos == nullptr)
//	{
//		buffers.push_back(m_vb_Pos->GetBuffer().Get());
//	}
//	else
//	{
//		buffers.push_back(skinnedData->m_vs_SkinnedPos->GetBuffer().Get());
//	}
//	
//	// 1 : UV
//	if (m_vb_UV != nullptr)
//	{
//		buffers.push_back(m_vb_UV->GetBuffer().Get());
//	}
//	else
//	{
//		buffers.push_back(nullptr);
//	}
//	
//	// 2 : Tangent
//	if (m_vb_Tangent != nullptr)
//	{
//		// スキニング済み頂点バッファがない時は、通常の頂点バッファをセット
//		if (skinnedData == nullptr || skinnedData->m_vs_SkinnedTangent == nullptr)
//		{
//			buffers.push_back(m_vb_Tangent->GetBuffer().Get());
//		}
//		// スキニング済み頂点バッファがある時は、それをセット
//		else
//		{
//			buffers.push_back(skinnedData->m_vs_SkinnedTangent->GetBuffer().Get());
//		}
//	}
//	else
//	{
//		buffers.push_back(nullptr);
//	}
//
//	// 3 : Normal
//	if (m_vb_Normal != nullptr)
//	{
//		if (skinnedData == nullptr || skinnedData->m_vs_SkinnedNormal == nullptr)
//		{
//			buffers.push_back(m_vb_Normal->GetBuffer().Get());
//		}
//		else
//		{
//			buffers.push_back(skinnedData->m_vs_SkinnedNormal->GetBuffer().Get());
//		}
//	}
//	else
//	{
//		buffers.push_back(nullptr);
//	}
//
//	// 4 : Color
//	if (m_vb_Color != nullptr)
//	{
//		buffers.push_back(m_vb_Color->GetBuffer().Get());
//	}
//	else
//	{
//		buffers.push_back(nullptr);
//	}
//
//	/*
//	// 5 : 
//	if (m_vb_SkinIndex != nullptr)
//		buffers.push_back(m_vb_SkinIndex->GetBuffer().Get());
//	else
//		buffers.push_back(nullptr);
//
//	// 6 : 
//	if (m_vb_SkinWeight != nullptr)
//		buffers.push_back(m_vb_SkinWeight->GetBuffer().Get());
//	else
//		buffers.push_back(nullptr);
//	*/
//
//	D3D.GetDevContext()->IASetVertexBuffers(0, (uint32_t)buffers.size(), &buffers[0], &m_stride[0], &m_offset[0]);
//
//	// インデックスバッファセット
//	D3D.GetDevContext()->IASetIndexBuffer(m_ib->GetBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
//
//	//プリミティブ・トポロジーをセット
//	D3D.GetDevContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//}
//
//void KdMesh::GPUSkinning(KdMeshGPUSkinningData& skinningData) const
//{
//	// スキンメッシュのみ
//	if (IsSkinMesh() == false)return;
//
//	auto* buffer = m_vb_Pos->GetBuffer().Get();
//
//	// 使用する頂点バッファ
//	std::array<ID3D11Buffer*, 5> buffers =
//	{
//		m_vb_Pos->GetBuffer().Get(),		// 座標
//		m_vb_Tangent->GetBuffer().Get(),	// 接線
//		m_vb_Normal->GetBuffer().Get(),		// 法線
//		m_vb_SkinIndex->GetBuffer().Get(),	// スキンインデックス
//		m_vb_SkinWeight->GetBuffer().Get(),	// スキンウェイト
//	};
//	// 各頂点バッファの、１項目のバイト数
//	std::array<UINT, 5>			stride = {12, 12, 12, 8, 16};
//	std::array<UINT, 5>			offset = {0, 0, 0, 0, 0};
//
//	// 頂点バッファたちをセット
//	D3D.GetDevContext()->IASetVertexBuffers(0, (uint32_t)buffers.size(), &buffers[0], &stride[0], &offset[0]);
//	// インデックスバッファをセット
//	D3D.GetDevContext()->IASetIndexBuffer(m_ib->GetBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
//
//	// Stream Output用、出力先頂点バッファ
//	std::vector<ID3D11Buffer*> outputBuffers =
//	{
//		skinningData.m_vs_SkinnedPos->GetBuffer().Get(),		// 座標 書き込み先
//		skinningData.m_vs_SkinnedTangent->GetBuffer().Get(),	// 接線 書き込み先
//		skinningData.m_vs_SkinnedNormal->GetBuffer().Get(),		// 法線 書き込み先
//	};
//
//	// GPUスキニングを実行
//	KdShaderManager::GetInstance().m_skinningShader.DrawToStreamOutput(m_vertexCount, outputBuffers);
//
//	// 頂点バッファたちを解除
//	std::array<ID3D11Buffer*, buffers.size()> nullBuffer = {};
//	D3D.GetDevContext()->IASetVertexBuffers(0, (uint32_t)buffers.size(), &nullBuffer[0], &stride[0], &offset[0]);
//}
//
//void KdMesh::DrawSubset(uint32_t subsetNo) const
//{
//	// 範囲外のサブセットはスキップ
//	if (subsetNo >= (int)m_subsets.size())return;
//	// 面数が0なら描画スキップ
//	if (m_subsets[subsetNo].FaceCount == 0)return;
//
//	// 描画
//	D3D.GetDevContext()->DrawIndexed(m_subsets[subsetNo].FaceCount * 3, m_subsets[subsetNo].FaceStart * 3, 0);
//}
//
//#endif
//
//bool KdModel::Load(const std::string& filepath)
//{
//	Release();
//
//	//------------------------------
//	// メッシュモデルファイル読み込み
//	//------------------------------
//	KdMeshScene scene;
//	if (scene.LoadMesh(filepath) == false) return false;
//
//	// パス
//	std::filesystem::path path = filepath;
//	std::string parentPath = path.parent_path().generic_string();
//
//	m_assetPath = filepath;
//
//	m_boneIndices = scene.m_boneIndices;
//	m_meshIndices = scene.m_meshIndices;
//
//	//------------------------------
//	// マテリアル設定
//	//------------------------------
//	for (auto&& material : scene.m_materials)
//	{
//		if (material.Name == "")continue;
//
//		// シェーダー
//		auto shader = KdShaderManager::GetInstance().GetShader("Lit");
//
//		// マテリアル作成
//		auto mate = shader->CreateMaterial();
//
//		mate->SetData(material.Name);
//
//		// 基本色
//		mate->SetValue(0, "g_BaseColor", material.BaseColor);
//		// メタリック
//		mate->SetValue(0, "g_Metallic", material.Metallic);
//		// スムーズネス
//		mate->SetValue(0, "g_Smoothness", material.Smoothness);
//		// 放射色
//		mate->SetValue(0, "g_EmissiveColor", material.EmissiveColor);
//
//		// 0番目のパス固定で使用
//		auto shaderPass = shader->GetPass(0);
//
//		// テクスチャ
//		for (auto&& info : shaderPass->GetResouceInputInfos())
//		{
//			// 基本テクスチャ
//			if (info.second.m_name == "g_baseMap")
//			{
//				auto tex = KdResourceManager::GetInstance().LoadAsset<KdTexture>(parentPath + "\\" + material.BaseMapFilename);
//				if (tex != nullptr)
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, tex);
//				}
//				else
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, D3D.GetDotWhiteTex());
//				}
//			}
//			else if (info.second.m_name == "g_normalMap")
//			{
//				auto tex = KdResourceManager::GetInstance().LoadAsset<KdTexture>(parentPath + "\\" + material.NormalMapFilename);
//				if (tex != nullptr)
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, tex);
//				}
//				else
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, D3D.GetDotNormalTex());
//				}
//			}
//			else if (info.second.m_name == "g_metallicSmoothnesMap")
//			{
//				std::filesystem::path baseTexPath = material.BaseMapFilename;
//				std::string smMapFilename = baseTexPath.stem().generic_string() + ".ms" + baseTexPath.extension().generic_string();
//
//
//				auto tex = KdResourceManager::GetInstance().LoadAsset<KdTexture>(parentPath + "\\" + smMapFilename);
//				if (tex != nullptr)
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, tex);
//				}
//				else
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, D3D.GetDotWhiteTex());
//				}
//			}
//			else if (info.second.m_name == "g_emissiveMap")
//			{
//				auto tex = KdResourceManager::GetInstance().LoadAsset<KdTexture>(parentPath + "\\" + material.EmissiveMapFilename);
//				if (tex != nullptr)
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, tex);
//				}
//				else
//				{
//					mate->SetTexture(0, info.second.m_bindIdx, D3D.GetDotWhiteTex());
//				}
//			}
//
//		}
//
//		m_materials.push_back(mate);
//	}
//
//	//------------------------------
//	// ノード設定
//	//------------------------------
//
//	m_allNodes.clear();
//	m_allNodes.resize(scene.m_allNodes.size());
//
//	for (uint32_t i = 0; i < scene.m_allNodes.size(); i++)
//	{
//		auto& srcNode = scene.m_allNodes[i];
//		Node& node = m_allNodes[i];
//
//		node.NodeName = srcNode.NodeName;
//		node.NodeIndex = srcNode.NodeIndex;
//		node.BoneIndex = srcNode.BoneIndex;
//		node.Children = srcNode.Children;
//
//		node.ParentIndex = srcNode.ParentIndex;
//
//		node.Transform.SetLocalMatrix(srcNode.LocalTransform);
//		node.BoneOffsetMatrix = srcNode.BoneOffsetMatrix;
//
//		// メッシュ作成
//		if (srcNode.m_meshDatas.empty() == false)
//		{
//			node.Mesh = std::make_shared<KdMesh>();
//			if (node.Mesh->Create(parentPath, srcNode.m_meshDatas))
//			{
//				if (node.Mesh->IsSkinMesh())
//				{
//					m_hasSkinMesh = true;
//				}
//			}
//		}
//
//		// 親を参照
//		if (srcNode.ParentIndex >= 0)
//		{
//			node.Transform.m_parent = &m_allNodes[srcNode.ParentIndex].Transform;
//		}
//
//		// 子を参照
//		for (size_t iChild = 0; iChild < srcNode.Children.size(); iChild++)
//		{
//			node.Transform.m_children.push_back(&m_allNodes[iChild].Transform);
//		}
//	}
//
//	// アニメーションデータコピー
//	m_animations = scene.m_animations;
//
//	return true;
//}
