#include "ModelLoader.h"
#include "FlModelImportData.h"

void ModelLoader::BuildNodeHierarchy(aiNode* aiNode, ModelData& model, int32_t parentIndex,
	std::map<std::string, int32_t>& nodeNameToIndex)
{
	auto node = std::make_shared<ModelData::Node>();
	node->m_name = aiNode->mName.C_Str();
	node->m_mLocal = *reinterpret_cast<Math::Matrix*>(&aiNode->mTransformation.Transpose());
	node->m_nodeIndex = static_cast<int32_t>(model.WorkNodes().size());
	node->m_parentIndex = parentIndex;

	nodeNameToIndex[node->m_name] = node->m_nodeIndex;

	model.WorkNodes().push_back(*node);

	if (node->m_parentIndex >= 0)
		model.WorkNodes()[node->m_parentIndex].m_children.push_back(node->m_nodeIndex);

	for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
		BuildNodeHierarchy(aiNode->mChildren[i], model, node->m_nodeIndex, nodeNameToIndex);
	}
}

void ModelLoader::BuildNodeMeshes(const aiNode* sourceNode, const aiScene* scene,
    const std::string& directory, ModelData& model,
    const std::map<std::string, int32_t>& nodeNameToIndex, const int32_t nodeIndex)
{
    for (unsigned int mesh = 0; mesh < sourceNode->mNumMeshes; ++mesh) {
        auto meshNodeIndex = nodeIndex;
        if (mesh > 0) {
            ModelData::Node child;
            child.m_name = model.WorkNodes()[nodeIndex].m_name + "#mesh" + std::to_string(mesh);
            child.m_mLocal = Def::Mat;
            child.m_nodeIndex = static_cast<int32_t>(model.WorkNodes().size());
            child.m_parentIndex = nodeIndex;
            meshNodeIndex = child.m_nodeIndex;
            model.WorkNodes().push_back(std::move(child));
            model.WorkNodes()[nodeIndex].m_children.push_back(meshNodeIndex);
        }
        const auto sourceMesh = scene->mMeshes[sourceNode->mMeshes[mesh]];
        const auto material = scene->mMaterials[sourceMesh->mMaterialIndex];
        model.WorkMeshNodeIndices().push_back(meshNodeIndex);
        model.WorkNodes()[meshNodeIndex].m_spMesh = Parse(scene, sourceMesh, material, directory, model, nodeNameToIndex);
    }
    for (unsigned int child = 0; child < sourceNode->mNumChildren; ++child)
        BuildNodeMeshes(sourceNode->mChildren[child], scene, directory, model, nodeNameToIndex, model.GetNodes()[nodeIndex].m_children[child]);
}

bool ModelLoader::Load(std::string filepath, ModelData& model)
{
	Assimp::Importer importer;
	auto flag = aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace | aiProcess_MakeLeftHanded | aiProcess_LimitBoneWeights;

	const auto pScene = importer.ReadFile(filepath, flag);
	if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode) {
		return false;
	}

	auto dirPath = std::filesystem::path(filepath).parent_path().generic_string() + "/";

	// Build the hierarchy before assigning bones and creating meshes.
	std::map<std::string, int32_t> nodeNameToIndex;
	model.WorkNodes().clear();
	model.WorkMeshNodeIndices().clear();
	model.WorkBoneNodeIndices().clear();
	model.WorkAnimation().clear();
	model.SetIsSkinMesh(false);
	BuildNodeHierarchy(pScene->mRootNode, model, -1, nodeNameToIndex);

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
				if (it == nodeNameToIndex.end()) return false;
				if (model.WorkNodes()[it->second].m_boneIndex >= 0) continue;
				if (static_cast<size_t>(boneIndex) >= CBufferData::BoneTransforms{}.boneTransforms.size()) return false;
				{
					model.WorkNodes()[it->second].m_boneIndex = boneIndex;
					model.WorkBoneNodeIndices().push_back(it->second);
					model.WorkNodes()[it->second].m_mBoneInverseWorld =
						*reinterpret_cast<Math::Matrix*>(&bone->mOffsetMatrix.Transpose());
					++boneIndex;
				}
			}
		}
	}

	BuildNodeMeshes(pScene->mRootNode, pScene, dirPath, model, nodeNameToIndex, 0);

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

			BindAnimationChannel(srcChannel, nodeNameToIndex);
		}
		spAnimationDatas.emplace_back(spAnimaData);
	}

	return true;
}

std::shared_ptr<Mesh> ModelLoader::Parse(const aiScene* pScene, const aiMesh* pMesh, 
	const aiMaterial* pMaterial, const std::string& dirPath, ModelData& model, 
	const std::map<std::string, int32_t>& nodeNameToIndex) 
{
	auto vertices = ParseMeshVertices(*pMesh, model, nodeNameToIndex);
	auto faces(std::vector<MeshFace>(pMesh->mNumFaces));

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
