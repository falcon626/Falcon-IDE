#include "FlMeshLoader.h"

bool MeshScene::LoadMesh(const std::string& filename)
{
	Clear();

	m_filepath = filename;

	//--------------------------------
	// モデルロード(assimp)
	//--------------------------------
	Assimp::Importer importer;
	uint32_t flag = aiProcess_Triangulate |
					aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder |
					aiProcess_CalcTangentSpace | aiProcess_FlipUVs;
	auto pScene = importer.ReadFile(filename, flag);
	if (pScene == nullptr)
	{
		m_filepath = "";
		return false;
	}

	// あとでメッシュを作成する用の仮データ
	struct TempCreateMeshData
	{
		aiNode*		SrcNode = nullptr;
		int			DestNodeIndex = 0;
	};
	std::vector<TempCreateMeshData> tempCreateMeshDatas;

	//--------------------------------
	// マテリアルリスト
	//--------------------------------
	m_materials.resize(pScene->mNumMaterials);
	for (uint32_t iMate = 0; iMate < pScene->mNumMaterials; iMate++)
	{
		auto pMate = pScene->mMaterials[iMate];

		MaterialData& destMaterial = m_materials[iMate];

		std::vector<std::string> keys;
		for (uint32_t i = 0; i < pMate->mNumProperties; i++)
		{
			auto prop = pMate->mProperties[i];
			keys.push_back(prop->mKey.C_Str());
		}

		// 名前
		aiString name;
		if (pMate->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
		{
			destMaterial.Name = name.C_Str();
		}

		// 基本テクスチャ名
		aiString path;
		if (pMate->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
		{
			destMaterial.BaseMapFilename = path.C_Str();
		}

		// 基本色
		aiColor4D color;
		if (pMate->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
		{
			destMaterial.BaseColor = { color.r, color.g, color.b, color.a };
		}
		float alpha = 1;
		if (pMate->Get(AI_MATKEY_OPACITY, alpha) == AI_SUCCESS)
		{
			destMaterial.BaseColor.w = alpha;
		}

		// 法線マップ
		if (pMate->Get(AI_MATKEY_TEXTURE_NORMALS(0), path) == AI_SUCCESS)
		{
			destMaterial.NormalMapFilename = path.C_Str();
		}

		// 放射マップ
		if (pMate->Get(AI_MATKEY_TEXTURE_EMISSIVE(0), path) == AI_SUCCESS)
		{
			destMaterial.EmissiveMapFilename = path.C_Str();
		}
		if (pMate->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS)
		{
			destMaterial.EmissiveColor = { color.r, color.g, color.b, color.a };
		}
	}

	//--------------------------------
	// 全ノード作成
	//--------------------------------
	std::function<void(aiNode*, int)> RecNode = [this, &RecNode, &tempCreateMeshDatas](aiNode* node, int parentNodeIndex)
		{
			int nodeIndex = (int)m_allNodes.size();

			NodeData nodeData;
			nodeData.NodeName = node->mName.C_Str();
			nodeData.NodeIndex = nodeIndex;
			nodeData.ParentIndex = parentNodeIndex;
			nodeData.LocalTransform = *(Math::Matrix*)&node->mTransformation.Transpose();
			m_allNodes.push_back(nodeData);

			if (parentNodeIndex >= 0)
			{
				m_allNodes[parentNodeIndex].Children.push_back(nodeIndex);
			}

			// メッシュ作成
			if (node->mNumMeshes > 0)
			{
				// メッシュは後で作成するため。情報を記憶しておく
				tempCreateMeshDatas.push_back({ node, nodeIndex });

				// メッシュのIndexを記憶
				m_meshIndices.push_back(nodeIndex);
			}

			// 子再帰
			for (uint32_t iChild = 0; iChild < node->mNumChildren; iChild++)
			{
				RecNode(node->mChildren[iChild], nodeIndex);
			}
		};

	RecNode(pScene->mRootNode, -1);

	//--------------------------------
	// メッシュデータを作成
	//--------------------------------
	for (auto&& createNodeData : tempCreateMeshDatas)
	{
		// 元データ
		aiNode* srcNode = createNodeData.SrcNode;
		// 保存先データ
		NodeData* destNodeData = &m_allNodes[createNodeData.DestNodeIndex];

		// メッシュ数
		if (srcNode->mNumMeshes > 0)
		{
			// メッシュ数だけ作成
			destNodeData->m_meshDatas.resize(srcNode->mNumMeshes);

			// 全メッシュ
			for (uint32_t mi = 0; mi < srcNode->mNumMeshes; mi++)
			{
				uint32_t srcMeshIdx = srcNode->mMeshes[mi];

				// 元メッシュデータ
				aiMesh* pMeshData = pScene->mMeshes[srcMeshIdx];
				// 格納先データの参照
				auto& outMeshData = destNodeData->m_meshDatas[mi];

				// 名前 (同じ名前のメッシュは、あとで結合とかしても良い)
				destNodeData->m_meshDatas[mi].Name = pMeshData->mName.C_Str();

				//---------------------------------------------------
				// 頂点データ
				//---------------------------------------------------
				outMeshData.Vertex.PosList.resize(pMeshData->mNumVertices);
				if (pMeshData->HasNormals()) outMeshData.Vertex.NormalList.resize(pMeshData->mNumVertices);
				if (pMeshData->HasTangentsAndBitangents()) outMeshData.Vertex.TangentList.resize(pMeshData->mNumVertices);
				if (pMeshData->HasVertexColors(0)) outMeshData.Vertex.ColorList.resize(pMeshData->mNumVertices);
				if (pMeshData->HasTextureCoords(0)) outMeshData.Vertex.UVList.resize(pMeshData->mNumVertices);

				for (uint32_t iV = 0; iV < pMeshData->mNumVertices; iV++)
				{
					// 座標
					outMeshData.Vertex.PosList[iV] = { pMeshData->mVertices[iV].x, pMeshData->mVertices[iV].y, pMeshData->mVertices[iV].z };

					// 法線
					if (pMeshData->HasNormals())
					{
						outMeshData.Vertex.NormalList[iV] = { pMeshData->mNormals[iV].x, pMeshData->mNormals[iV].y, pMeshData->mNormals[iV].z };
					}
					// 接線
					if (pMeshData->HasTangentsAndBitangents())
					{
						outMeshData.Vertex.TangentList[iV] = { pMeshData->mTangents[iV].x, pMeshData->mTangents[iV].y, pMeshData->mTangents[iV].z };
					}
					// 色
					if (pMeshData->HasVertexColors(0))
					{
						DirectX::SimpleMath::Color c(
							pMeshData->mColors[0][iV].r,
							pMeshData->mColors[0][iV].g,
							pMeshData->mColors[0][iV].b,
							pMeshData->mColors[0][iV].a
						);

						outMeshData.Vertex.ColorList[iV] = c.RGBA().v;
					}
					// UV
					if (pMeshData->HasTextureCoords(0))
					{
						outMeshData.Vertex.UVList[iV] = { pMeshData->mTextureCoords[0][iV].x, pMeshData->mTextureCoords[0][iV].y };
					}
				}

				// 面データ
				outMeshData.FaceDatas.resize(pMeshData->mNumFaces);
				for (uint32_t i = 0; i < pMeshData->mNumFaces; ++i)
				{
					const auto& face = pMeshData->mFaces[i];

					outMeshData.FaceDatas[i].Idx[0] = face.mIndices[0];
					outMeshData.FaceDatas[i].Idx[1] = face.mIndices[1];
					outMeshData.FaceDatas[i].Idx[2] = face.mIndices[2];
				}

				// ボーン
				if (pMeshData->HasBones())
				{
					// 頂点のウェイト関係の情報を作成
					if (outMeshData.Vertex.SkinWeightList.size() == 0)
					{
						outMeshData.Vertex.SkinWeightList.resize(pMeshData->mNumVertices);
						outMeshData.Vertex.SkinIndexList.resize(pMeshData->mNumVertices);
						for (auto&& p : outMeshData.Vertex.SkinIndexList)
						{
							p[0] = p[1] = p[2] = p[3] = -1;
						}
					}

					// 全ボーン
					m_boneIndices.resize(pMeshData->mNumBones);
					for (uint32_t iBone = 0; iBone < pMeshData->mNumBones; iBone++)
					{
						aiBone* srcBone = pMeshData->mBones[iBone];

						// ボーン名から、ノードを検索
						std::string boneName = srcBone->mName.C_Str();

						auto itFoundNode = std::find_if(m_allNodes.begin(), m_allNodes.end(), [boneName](const NodeData& nodeData) {return nodeData.NodeName == boneName; });
						if (itFoundNode != m_allNodes.end())
						{
							m_boneIndices[iBone] = (uint32_t)std::distance(m_allNodes.begin(), itFoundNode);

							// ノード発見、情報を入れる
							itFoundNode->BoneIndex = (int)iBone;
							itFoundNode->BoneOffsetMatrix = *(Math::Matrix*)&srcBone->mOffsetMatrix.Transpose();

							// このボーンに影響している頂点にウェイト情報を作成
							for (uint32_t iW = 0; iW < srcBone->mNumWeights; iW++)
							{
								// 対象の頂点Index
								uint32_t vertexIdx = srcBone->mWeights[iW].mVertexId;

								auto& destBoneIdxArray = outMeshData.Vertex.SkinIndexList[vertexIdx];
								auto& destBoneWeightArray = outMeshData.Vertex.SkinWeightList[vertexIdx];

								// 指定頂点の空きウェイト場所を探し、そこに値をいれる
								for (size_t iDest = 0; iDest < destBoneIdxArray.size(); iDest++)
								{
									// 空いているか？
									if (destBoneIdxArray[iDest] == -1)
									{
										// この頂点が影響するボーン番号
										destBoneIdxArray[iDest] = iBone;
										// ウェイト値
										destBoneWeightArray[iDest] = srcBone->mWeights[iW].mWeight;
										break;
									}
								}
							}
						}

					}
				}

				// 頂点ウェイトを正規化
				for (uint32_t iV = 0; iV < outMeshData.Vertex.SkinIndexList.size(); iV++)
				{
					uint32_t numWeights = 0;
					float totalWeight = 0;
					// ウェイトLoop
					for (numWeights = 0; numWeights < outMeshData.Vertex.SkinIndexList[iV].size(); numWeights++)
					{
						// ウェイト合計を求める
						if (outMeshData.Vertex.SkinIndexList[iV][numWeights])
						{
							totalWeight += outMeshData.Vertex.SkinWeightList[iV][numWeights];
						}
					}

					// 最後のウェイトを調整
					for (uint32_t idx = 0; idx < numWeights; idx++)
					{
						outMeshData.Vertex.SkinWeightList[iV][idx] /= totalWeight;
					}
				}

				// マテリアルIndex
				outMeshData.MaterialIndex = pMeshData->mMaterialIndex;
			}

		}
	}

	// Animation
	//if( pScene->HasAnimations() )
	//{
	//	UINT animNum = pScene->mNumAnimations; // アニメーションの数

	//	for(UINT cnt = 0; cnt < animNum; cnt++)
	//	{
	//		KdAnimationData destAnim;
	//		destAnim.Parse(pScene->mAnimations[cnt], m_allNodes);
	//		m_animations.push_back(destAnim);
	//	}
	//}

	return true;
}