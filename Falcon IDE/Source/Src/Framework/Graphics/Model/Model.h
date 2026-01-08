#pragma once

struct AnimationData;

class ModelData
{
public:

	struct Node
	{
		std::string				m_name;

		std::shared_ptr<Mesh>	m_spMesh;	// メッシュ
		
		Math::Matrix				m_mLocal;
		Math::Matrix			m_mBoneInverseWorld; // 原点からの逆行列

		int32_t                 m_nodeIndex = -Def::IntOne;
		int32_t					m_boneIndex = -Def::IntOne;

		std::vector<int32_t>    m_children;
		int32_t                 m_parentIndex = -Def::IntOne;
	};

	/// <summary>
	/// モデルのロード
	/// </summary>
	/// <param name="filepath">ファイルパス</param>
	/// <returns>成功したらtrue</returns>
	bool Load(const std::string& filepath);

	/// <summary>
	/// ノードの取得
	/// </summary>
	/// <returns>ノード情報</returns>
	const std::vector<Node>& GetNodes()const noexcept { return m_nodes; }
	std::vector<Node>& WorkNodes() noexcept { return m_nodes; }

	const std::vector<int>& GetMeshNodeIndices() const noexcept { return m_meshNodeIndices; }
	std::vector<int>& WorkMeshNodeIndices() noexcept { return m_meshNodeIndices; }

	const std::vector<int>& GetBoneNodeIndices() const noexcept { return m_boneNodeIndices; }
	std::vector<int>& WorkBoneNodeIndices() noexcept { return m_boneNodeIndices; }

	const std::shared_ptr<AnimationData> GetAnimation(const std::string& animName) const;
	const std::shared_ptr<AnimationData> GetAnimation(UINT index) const;

	const auto GetAnimetionsSize() { return m_spAnimations.size(); }

	std::vector<std::shared_ptr<AnimationData>>& WorkAnimation() noexcept { return m_spAnimations; }

	inline const auto SetIsSkinMesh(const bool is) noexcept { m_isSkinMesh = is; }

	inline const auto IsSkinMesh() const noexcept { return m_isSkinMesh; }

private:

	std::vector<Node>		m_nodes;
	std::vector<std::shared_ptr<AnimationData>> m_spAnimations;

	// 全ノード中、メッシュノードのみのIndex配列
	std::vector<int>		m_meshNodeIndices;

	// 全ノード中、ボーンノードのみのIndex配列
	std::vector<int>		m_boneNodeIndices;

	bool m_isSkinMesh = false;
};