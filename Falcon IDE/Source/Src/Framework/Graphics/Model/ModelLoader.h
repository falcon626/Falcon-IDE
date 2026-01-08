#pragma once

#include "Model.h"

class ModelLoader
{
public:

	/// <summary>
	/// モデルのロード
	/// </summary>
	/// <param name="filepath">ファイルパス</param>
	/// <param name="nodes">ノード情報</param>
	/// <returns>成功したらtrue</returns>
	bool Load(std::string filepath, ModelData& model);

private:

	/// <summary>
	/// 解析
	/// </summary>
	/// <param name="pScene">モデルシーンのポインタ</param>
	/// <param name="pMesh">メッシュのポインタ</param>
	/// <param name="pMaterial">マテリアルのポインタ</param>
	/// <param name="dirPath">ディレクトリパス</param>
	/// <returns>メッシュポインタ</returns>
	std::shared_ptr<Mesh> Parse(const aiScene* pScene, const aiMesh* pMesh, 
		const aiMaterial* pMaterial, const std::string& dirPath, ModelData& model, 
		const std::map<std::string, int32_t>& nodeNameToIndex);

	/// <summary>
	/// マテリアルの解析
	/// </summary>
	/// <param name="pMaterial">マテリアルのポインタ</param>
	/// <param name="dirPath">ディレクトリパス</param>
	/// <returns>マテリアル情報</returns>
	const Material ParseMaterial(const aiMaterial* pMaterial, const std::string& dirPath);

	void BuildNodeHierarchy(aiNode* aiNode, ModelData& model, int32_t parentIndex,
		std::map<std::string, int32_t>& nodeNameToIndex,
		const aiScene* pScene, const std::string& dirPath) noexcept;
};