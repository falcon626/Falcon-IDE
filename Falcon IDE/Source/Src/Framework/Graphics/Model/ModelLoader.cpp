#include "ModelLoader.h"

void ModelLoader::BuildNodeHierarchy(aiNode* aiNode, ModelData& model, int32_t parentIndex,
	std::map<std::string, int32_t>& nodeNameToIndex,
	const aiScene* pScene, const std::string& dirPath) noexcept
{
	auto node = std::make_shared<ModelData::Node>();
	node->m_name = utf8_to_ansi(aiNode->mName.C_Str());
	node->m_mLocal = *reinterpret_cast<Math::Matrix*>(&aiNode->mTransformation.Transpose());
	node->m_nodeIndex = static_cast<int32_t>(model.WorkNodes().size());
	node->m_parentIndex = parentIndex;

	nodeNameToIndex[node->m_name] = node->m_nodeIndex;

	model.WorkNodes().push_back(*node);

	if (node->m_parentIndex >= 0)
		model.WorkNodes()[node->m_parentIndex].m_children.push_back(node->m_nodeIndex);

	for (unsigned int i = 0; i < aiNode->mNumMeshes; ++i) {
		unsigned int meshIndex = aiNode->mMeshes[i];
		auto pMesh = pScene->mMeshes[meshIndex];
		auto pMaterial = pScene->mMaterials[pMesh->mMaterialIndex];

		model.WorkMeshNodeIndices().push_back(node->m_nodeIndex);
		model.WorkNodes()[node->m_nodeIndex].m_spMesh =
			Parse(pScene, pMesh, pMaterial, dirPath, model, nodeNameToIndex);
	}

	// 再帰的に子ノード処理
	for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
		BuildNodeHierarchy(aiNode->mChildren[i], model, node->m_nodeIndex, nodeNameToIndex, pScene, dirPath);
	}
}

bool ModelLoader::Load(std::string filepath, ModelData& model)
{
	Assimp::Importer importer;
	auto flag = aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace | aiProcess_MakeLeftHanded;

	const auto pScene = importer.ReadFile(filepath, flag);
	if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode) {
		return false;
	}

	auto dirPath = std::filesystem::path(filepath).parent_path().generic_string() + "/";

	// ノード階層を構築（ここでメッシュ割り当ても完了する）
	std::map<std::string, int32_t> nodeNameToIndex;
	model.WorkNodes().clear();
	BuildNodeHierarchy(pScene->mRootNode, model, -1, nodeNameToIndex, pScene, dirPath);

	// ボーン情報の設定
	int32_t boneIndex = 0;
	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
		auto pMesh = pScene->mMeshes[i];
		if (pMesh->HasBones()) {
			model.SetIsSkinMesh(true);
			for (unsigned int b = 0; b < pMesh->mNumBones; ++b) {
				auto bone = pMesh->mBones[b];
				auto boneName = std::string{ bone->mName.C_Str() };
				auto it = nodeNameToIndex.find(boneName);
				if (it != nodeNameToIndex.end()) {
					model.WorkNodes()[it->second].m_boneIndex = boneIndex;
					model.WorkBoneNodeIndices().push_back(it->second);
					model.WorkNodes()[it->second].m_mBoneInverseWorld =
						*reinterpret_cast<Math::Matrix*>(&bone->mOffsetMatrix.Transpose());
					++boneIndex;
				}
			}
		}
	}

	// アニメーションデータの解析
	auto& spAnimationDatas = model.WorkAnimation();
	for (unsigned int i = 0; i < pScene->mNumAnimations; ++i) {
		auto pAnimation = pScene->mAnimations[i];
		auto spAnimaData = std::make_shared<AnimationData>();
		spAnimaData->m_name = pAnimation->mName.C_Str();
		spAnimaData->m_maxTime = static_cast<float>(pAnimation->mDuration);
		spAnimaData->m_channels.resize(pAnimation->mNumChannels);

		for (unsigned int j = 0; j < pAnimation->mNumChannels; ++j) {
			auto& srcChannel = spAnimaData->m_channels[j];
			srcChannel.m_name = pAnimation->mChannels[j]->mNodeName.C_Str();
			auto dstChannel = pAnimation->mChannels[j];

			for (unsigned int k = 0; k < dstChannel->mNumPositionKeys; ++k) {
				auto translation = AnimKeyVector3{};
				translation.m_time = static_cast<float>(dstChannel->mPositionKeys[k].mTime);
				translation.m_vec = Math::Vector3(dstChannel->mPositionKeys[k].mValue.x, dstChannel->mPositionKeys[k].mValue.y, dstChannel->mPositionKeys[k].mValue.z);
				srcChannel.m_translations.emplace_back(translation);
			}

			for (unsigned int k = 0; k < dstChannel->mNumRotationKeys; ++k) {
				auto rotation = AnimKeyQuaternion{};
				rotation.m_time = static_cast<float>(dstChannel->mRotationKeys[k].mTime);
				rotation.m_quat = Math::Quaternion(dstChannel->mRotationKeys[k].mValue.x, dstChannel->mRotationKeys[k].mValue.y, dstChannel->mRotationKeys[k].mValue.z, dstChannel->mRotationKeys[k].mValue.w);
				srcChannel.m_rotations.emplace_back(rotation);
			}

			for (unsigned int k = 0; k < dstChannel->mNumScalingKeys; ++k) {
				auto scale = AnimKeyVector3{};
				scale.m_time = static_cast<float>(dstChannel->mScalingKeys[k].mTime);
				scale.m_vec = Math::Vector3(dstChannel->mScalingKeys[k].mValue.x, dstChannel->mScalingKeys[k].mValue.y, dstChannel->mScalingKeys[k].mValue.z);
				srcChannel.m_scales.emplace_back(scale);
			}

			for (auto&& node : model.WorkNodes())
			{
				if (node.m_name == srcChannel.m_name) 
					srcChannel.m_nodeOffset = node.m_nodeIndex;
			}
		}
		spAnimationDatas.emplace_back(spAnimaData);
	}

	return true;
}

std::shared_ptr<Mesh> ModelLoader::Parse(const aiScene* pScene, const aiMesh* pMesh, 
	const aiMaterial* pMaterial, const std::string& dirPath, ModelData& model, 
	const std::map<std::string, int32_t>& nodeNameToIndex) 
{
	auto vertices{ MeshVertex{} };
	auto faces(std::vector<MeshFace>(pMesh->mNumFaces));

	if (pMesh->HasTextureCoords(0))vertices.UV.resize(pMesh->mNumVertices);
	if (pMesh->HasNormals())vertices.Normal.resize(pMesh->mNumVertices);
	if (pMesh->HasTangentsAndBitangents())vertices.Tangent.resize(pMesh->mNumVertices);
	if (pMesh->HasVertexColors(0))vertices.Color.resize(pMesh->mNumVertices);

#pragma omp parallel for
	for (auto i{ Def::UIntZero }; i < pMesh->mNumVertices; ++i) {
		vertices.Position.emplace_back(Math::Vector3(pMesh->mVertices[i].x, pMesh->mVertices[i].y, pMesh->mVertices[i].z));
		if (pMesh->HasTextureCoords(0)) {
			vertices.UV[i] = Math::Vector2(pMesh->mTextureCoords[0][i].x, pMesh->mTextureCoords[0][i].y);
		}
		if (pMesh->HasNormals()) {
			vertices.Normal[i] = Math::Vector3(pMesh->mNormals[i].x, pMesh->mNormals[i].y, pMesh->mNormals[i].z);
		}
		if (pMesh->HasTangentsAndBitangents()) {
			vertices.Tangent[i] = Math::Vector3(pMesh->mTangents[i].x, pMesh->mTangents[i].y, pMesh->mTangents[i].z);
		}
		if (pMesh->HasVertexColors(0)) {
			auto c{ Math::Color{pMesh->mColors[0][i].r, pMesh->mColors[0][i].g, pMesh->mColors[0][i].b, pMesh->mColors[0][i].a} };
			vertices.Color[i] = c.RGBA().v;
		}

		// ボーンとウェイトの取得
		if (pMesh->HasBones()) {
			// 頂点データの初期化
			const unsigned int numVertices = pMesh->mNumVertices;
			vertices.SkinWeightList.resize(numVertices, { 0.0f, 0.0f, 0.0f, 0.0f });
			vertices.SkinIndexList.resize(numVertices, { 0, 0, 0, 0 });

			for (auto&& i : vertices.SkinIndexList)
			{
				i[0] = i[1] = i[2] = i[3] = -1;
			}

			for (unsigned int b = 0; b < pMesh->mNumBones; ++b) {
				const aiBone* bone = pMesh->mBones[b];
				std::string boneName = bone->mName.C_Str();

				auto it{ nodeNameToIndex.find(boneName) };

				if (it == nodeNameToIndex.end()) continue;
				int32_t boneIndex = model.WorkNodes()[it->second].m_boneIndex;

				for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
					const aiVertexWeight& weight = bone->mWeights[w];
					const unsigned int vertexId = weight.mVertexId;
					const float weightValue = weight.mWeight;

					// 範囲チェック
					if (vertexId >= numVertices) continue;

					// 空きスロットを探す
					for (int j = 0; j < vertices.SkinWeightList[vertexId].size(); ++j) {
						if (vertices.SkinIndexList[vertexId][j] == -1) {
							vertices.SkinWeightList[vertexId][j] = weightValue;
							vertices.SkinIndexList[vertexId][j]  = boneIndex;
							break;
						}
					}
				}
			}

			// ウェイトの正規化
			for (unsigned int v = 0; v < numVertices; ++v) {
				auto& weights = vertices.SkinWeightList[v];
				float sum = weights[0] + weights[1] + weights[2] + weights[3];

				if (sum > 0.0f) {
					for (int j = 0; j < 4; ++j) {
						weights[j] /= sum;
					}
				}
			}
		}
	}

	for (unsigned int i = 0; i < pMesh->mNumFaces; ++i) {
		faces[i].Idx[0] = pMesh->mFaces[i].mIndices[0];
		faces[i].Idx[1] = pMesh->mFaces[i].mIndices[1];
		faces[i].Idx[2] = pMesh->mFaces[i].mIndices[2];
	}

	auto spMesh{ std::make_shared<Mesh>() };
	spMesh->SetInputLayout(Shader::Instance().GetInputLayout());
	spMesh->Create(&GraphicsDevice::Instance(), vertices, faces, ParseMaterial(pMaterial, dirPath), pMesh->mNumVertices);
	return spMesh;
}

const Material ModelLoader::ParseMaterial(const aiMaterial* pMaterial, const std::string& dirPath)
{
	if (Shader::Instance().GetSRVCount() == Def::UIntZero) return Material();
	auto material{ Material {} };
	// マテリアルの名前を取得
	{
		aiString name;
		if (pMaterial->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
		{
			material.Name = name.C_Str();
		}
	}
	// doubleSidedを取得
	{
		bool doubleSided = false;
		if (pMaterial->Get(AI_MATKEY_TWOSIDED, doubleSided) == AI_SUCCESS)
		{
			material.doubleSided = doubleSided;
		}
	}
	// Diffuseテクスチャの取得
	{
		aiString path;
		if (pMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &path) == AI_SUCCESS)
		{
			auto filePath = std::string(path.C_Str());
			material.spBaseColorTex = std::make_shared<Texture>(); 
				if (!material.spBaseColorTex->Load(dirPath + filePath))
				{
					assert(0 && "Diffuseテクスチャのロードに失敗");
					return Material();
				}
		}
		if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
		{
			auto filePath = std::string(path.C_Str());
			material.spBaseColorTex = std::make_shared<Texture>();
				if (!material.spBaseColorTex->Load(dirPath + filePath))
				{
					assert(0 && "Diffuseテクスチャのロードに失敗");
					return Material();
				}
		}
	}
	// BaseColorFactorの取得
	{
		aiColor4D baseColor;
		if (pMaterial->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
		{
			material.BaseColor.x = baseColor.r;
			material.BaseColor.y = baseColor.g;
			material.BaseColor.z = baseColor.b;
			material.BaseColor.w = baseColor.a;
		}
		else
		{
			// 従来形式
			aiColor4D diffuse;
			if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
			{
				material.BaseColor.x = diffuse.r;
				material.BaseColor.y = diffuse.g;
				material.BaseColor.z = diffuse.b;
				material.BaseColor.w = diffuse.a;
			}
		}
	}
	// DiffuseColorの取得
	{
		aiColor4D diffuse;
		if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
		{
			material.BaseColor.x = diffuse.r;
			material.BaseColor.y = diffuse.g;
			material.BaseColor.z = diffuse.b;
			material.BaseColor.w = diffuse.a;
		}
	}
	// MetallicRoughnessテクスチャの取得
	{
		aiString path;
		if (pMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &path) == AI_SUCCESS ||
			pMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &path) == AI_SUCCESS)
		{
			auto filePath = std::string(path.C_Str());
			material.spMetallicRoughnessTex = std::make_shared<Texture>();
				if (!material.spMetallicRoughnessTex->Load(dirPath + filePath))
				{
					assert(false && "MetallicRoughnessテクスチャのロードに失敗");
					return Material();
				}
		}
	}
	// Metallicを取得
	{
		auto metallic{ Def::FloatZero };
		if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
		{
			material.Metallic = metallic;
		}
	}
	// Roughness
	{
		auto roughness{ Def::FloatOne };
		if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
		{
			material.Roughness = roughness;
		}
	}
	// Emissiveテクスチャの取得
	{
		auto path{ aiString{} };
		if (pMaterial->GetTexture(AI_MATKEY_EMISSIVE_TEXTURE, &path) == AI_SUCCESS)
		{
			auto filePath = std::string(path.C_Str());
			material.spEmissiveTex = std::make_shared<Texture>();
				if (!material.spEmissiveTex->Load(dirPath + filePath))
				{
					assert(false && "Emissiveテクスチャのロードに失敗");
					return Material();
				}
		}
	}
	// Emissiveの取得
	{
		aiColor3D emissive;
		if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
		{
			material.Emissive.x = emissive.r;
			material.Emissive.y = emissive.g;
			material.Emissive.z = emissive.b;
		}
	}
	// 法線テクスチャの取得
	{
		aiString path;
		if (pMaterial->GetTexture(AI_MATKEY_NORMAL_TEXTURE, &path) == AI_SUCCESS)
		{
			auto filePath = std::string(path.C_Str());
			material.spNormalTex = std::make_shared<Texture>();
				if (!material.spNormalTex->Load(dirPath + filePath))
				{
					assert(false && "Normalテクスチャのロードに失敗");
					return Material();
				}
		}
	}
	return material;
}
