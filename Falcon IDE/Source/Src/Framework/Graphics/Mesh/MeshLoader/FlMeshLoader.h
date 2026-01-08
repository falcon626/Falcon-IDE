#pragma once

//==========================================================
// メッシュシーンデータ
// ・複数のメッシュなどが集まったもの
//==========================================================

struct KdAnimationData; 

class MeshScene
{
public :

	// 頂点データ
	struct VertexData
	{
		// 頂点データリスト
		std::vector<Math::Vector3>			PosList;			// 座標リスト
		std::vector<Math::Vector2>			UVList;				// UV座標リスト
		std::vector<Math::Vector3>			TangentList;		// 接線リスト
		std::vector<Math::Vector3>			NormalList;			// 法線リスト
		std::vector<uint32_t>			ColorList;			// 色リスト

		std::vector<std::array<short, 4>>	SkinIndexList;	// スキニングIndexリスト
		std::vector<std::array<float, 4>>	SkinWeightList;	// スキニングウェイトリスト

		// KdMeshVertexDataを結合する
		void Combine(const VertexData& vertex)
		{
			PosList.insert(PosList.end(), vertex.PosList.begin(), vertex.PosList.end());
			UVList.insert(UVList.end(), vertex.UVList.begin(), vertex.UVList.end());
			TangentList.insert(TangentList.end(), vertex.TangentList.begin(), vertex.TangentList.end());
			NormalList.insert(NormalList.end(), vertex.NormalList.begin(), vertex.NormalList.end());
			ColorList.insert(ColorList.end(), vertex.ColorList.begin(), vertex.ColorList.end());

			SkinIndexList.insert(SkinIndexList.end(), vertex.SkinIndexList.begin(), vertex.SkinIndexList.end());
			SkinWeightList.insert(SkinWeightList.end(), vertex.SkinWeightList.begin(), vertex.SkinWeightList.end());
		}

		void Clear()
		{
			PosList.clear();
			UVList.clear();
			TangentList.clear();
			NormalList.clear();
			ColorList.clear();
			SkinIndexList.clear();
			SkinWeightList.clear();
		}
	};
	// 面(Index)情報
	struct FaceData
	{
		uint32_t Idx[3];				// 三角形を構成する頂点のIndex
	};

	// マテリアルデータ
	struct MaterialData
	{
		// 基本情報
		std::string Name;

		// 基本テクスチャファイル名
		std::string BaseMapFilename;
		// 基本色
		Math::Color BaseColor;

		// 法線マップファイル名
		std::string NormalMapFilename;

		// 金属
		float Metallic = 0;
		// 滑らかさ
		float Smoothness = 0;

		// 放射色マップファイル名
		std::string EmissiveMapFilename;
		// 放射色
		Math::Color EmissiveColor;

	};

	// メッシュデータ(マテリアル単位で分割されている)
	struct MeshData
	{
		// メッシュ名
		std::string				Name;
		// 頂点データ
		VertexData				Vertex;
		// 面(Index)データ
		std::vector<FaceData>	FaceDatas;

		// マテリアル配列内のIndex
		uint32_t				MaterialIndex;

		void Clear()
		{
			Name = "";
			Vertex.Clear();
			FaceDatas.clear();
			MaterialIndex = 0;
		}
	};

	// マテリアルデータ
	std::vector<MaterialData>	m_materials;

	// AnimationData
	std::vector<KdAnimationData> m_animations;

	// ノードデータ
	struct NodeData
	{
		// 名前
		std::string				NodeName;

		// 自ノードIndex
		int						NodeIndex = -1;
		int						BoneIndex = -1;

		// 子Indexリスト
		std::vector<int>		Children;
		// 親Index
		int						ParentIndex = -1;

		// ローカル行列
		Math::Matrix					LocalTransform;

		// ボーンオフセット行列
		Math::Matrix					BoneOffsetMatrix;

		// メッシュリスト
		std::vector<MeshData>	m_meshDatas;
	};



public:

	// メッシュのパス
	std::filesystem::path	m_filepath;

	// 全ノード
	std::vector<NodeData>	m_allNodes;

	// 全ノード内、ボーンであるノードIndex集
	std::vector<uint32_t>	m_boneIndices;
	// 全ノード内、メッシュのあるノードIndex集
	std::vector<uint32_t>	m_meshIndices;


	// メッシュファイルを読み込む
	bool LoadMesh(const std::string& filename);

	void Clear()
	{
		m_filepath = "";
		m_allNodes.clear();
		m_boneIndices.clear();
		m_meshIndices.clear();
	}

};