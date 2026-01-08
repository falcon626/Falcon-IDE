//#pragma once
//
////==========================================================
//// 
//// ボーン事前スキニング用 頂点バッファ集
//// ・事前GPUスキニングで使用する
//// 
////==========================================================
//struct KdMeshGPUSkinningData
//{
//	std::shared_ptr<VertexBufferAllocator>		m_vs_SkinnedPos;		// スキニング済み 座標バッファ
//	std::shared_ptr<VertexBufferAllocator>		m_vs_SkinnedTangent;	// スキニング済み 接線バッファ
//	std::shared_ptr<VertexBufferAllocator>		m_vs_SkinnedNormal;		// スキニング済み 法線バッファ
//};
//
////==========================================================
//// サブセット情報
//// ・１メッシュ中、複数のマテリアルに分かれている場合、その情報を記憶したもの。
////==========================================================
//struct KdMeshSubset
//{
//	uint32_t		MaterialNo = 0;		// マテリアルNo
//
//	uint32_t		FaceStart = 0;		// 面Index　このマテリアルで使用されている最初の面のIndex
//	uint32_t		FaceCount = 0;		// 面数　FaceStartから、何枚の面が使用されているかの
//};
//
////==========================================================
////
//// 単一メッシュクラス
////
////==========================================================
//class KdMesh
//{
//public:
//
//	//=================================================
//	// 取得・設定
//	//=================================================
//
//	// 作成済み？
//	bool								IsCreated() const { return m_vb_Pos != nullptr; }
//
//	// 軸平行境界ボックス取得
//	const DirectX::BoundingBox&			GetBoundingBox() const { return m_aabb; }
//	// 境界球取得
//	const DirectX::BoundingSphere&		GetBoundingSphere() const { return m_bs; }
//
//	// スキンメッシュ？
//	bool								IsSkinMesh() const { return m_vb_SkinIndex != nullptr; }
//
////	std::shared_ptr<KdMaterial>			GetMaterial() { return m_material; }
//
//	const std::vector<KdMeshSubset>&	GetSubsets() const { return m_subsets; }
//
//	// 頂点情報の取得
//	const std::vector< KdVector3 >& GetVertexPositions() const {return m_vertexData.PosList;}
//
//	// 面情報の取得
//	const std::vector < KdMeshScene::FaceData >& GetFaces() const {return m_faceDatas;}
//
//	//=================================================
//	// 作成・解放
//	//=================================================
//
//	// メッシュ作成
//	bool Create(const std::string& parentDirPath, const std::vector<KdMeshScene::MeshData>& meshDatas);
//
//	// GPUスキニング用のデータを作成し、返す
//	std::shared_ptr<KdMeshGPUSkinningData> CreateGPUSkinningData();
//
//	// 解放
//	void Release()
//	{
//		m_vb_Pos = nullptr;
//		m_vb_UV = nullptr;
//		m_vb_Tangent = nullptr;
//		m_vb_Normal = nullptr;
//		m_vb_Color = nullptr;
//		m_vb_SkinIndex = nullptr;
//		m_vb_SkinWeight = nullptr;
//
//		m_ib = nullptr;
//
//		m_subsets.clear();
//
//		m_stride.clear();
//		m_offset.clear();
//
////		m_srcMeshDatas.clear();
//
//		m_vertexCount = 0;
//	}
//
//	~KdMesh()
//	{
//		Release();
//	}
//
//	//=================================================
//	// 描画関係
//	//=================================================
//
//	// メッシュデータをデバイスへセットする
//	void SetToDevice(const KdMeshGPUSkinningData* skinnedData) const;
//
//	// 頂点をスキニングし、別の頂点バッファに書き込む(Stream Output機能を使用)
//	void GPUSkinning(KdMeshGPUSkinningData& skinningData) const;
//
//	// 指定したサブセットを描画
//	void DrawSubset(uint32_t subsetNo) const;
//
//	// 
//	KdMesh() {}
//
//private:
//
//	//--------------------------
//	// 頂点バッファ
//	// ・要素ごとに頂点バッファを用意する
//	//--------------------------
//	std::shared_ptr<KdBuffer>		m_vb_Pos;		// 座標バッファ
//	std::shared_ptr<KdBuffer>		m_vb_UV;		// UV座標バッファ
//	std::shared_ptr<KdBuffer>		m_vb_Tangent;	// 接線バッファ
//	std::shared_ptr<KdBuffer>		m_vb_Normal;	// 法線バッファ
//	std::shared_ptr<KdBuffer>		m_vb_Color;		// 色バッファ
//	// ・スキンメッシュ用の時のみ作成する
//	std::shared_ptr<KdBuffer>		m_vb_SkinIndex;	// スキニングIndexバッファ
//	std::shared_ptr<KdBuffer>		m_vb_SkinWeight;// スキニングウェイトバッファ
//
//	// 各バッファごとの１項目のサイズ(例：PosならVector3なので12バイト)
//	std::vector<UINT>				m_stride;
//	std::vector<UINT>				m_offset;
//
//	// インデックスバッファ
//	std::shared_ptr<KdBuffer>		m_ib;
//
//	// サブセット情報
//	std::vector<KdMeshSubset>		m_subsets;
//
//	// 境界データ
//	DirectX::BoundingBox			m_aabb;	// 軸平行境界ボックス
//	DirectX::BoundingSphere			m_bs;	// 境界球
//
//	// 頂点数
//	uint32_t						m_vertexCount = 0;
//
//	// 全ての頂点情報と面情報
//	KdMeshScene::VertexData						m_vertexData;
//	std::vector< KdMeshScene::FaceData >		m_faceDatas;
//
//	// 元データ
////	std::vector<KdMeshScene::MeshData>	m_srcMeshDatas;
////	KdMeshScene::MeshData			m_srcMeshData;
//
//private:
//	// コピー禁止用
//	KdMesh(const KdMesh& src) = delete;
//	void operator=(const KdMesh& src) = delete;
//};
//
////==========================================================
//// 
//// FBXやBlenderのような、複数のメッシュやノードの階層構造などを全て持ったもの
//// 
//// ・階層ノード構造		：行列やメッシュを持ち、階層構造になっているObject
//// ・複数メッシュ		：描画可能な複数のメッシュをもつ(ノード内に入っている)
//// ・マテリアルリスト	：使用している全てのマテリアルのリスト
//// 
////==========================================================
//class KdModel : public KdObject
//{
//public:
//
//	struct Node;
//
//	// アセットパス取得
//	const std::string& GetAssetPath() const { return m_assetPath; }
//
//	// スキンメッシュを持っている？
//	bool HasSkinMesh() const { return m_hasSkinMesh; }
//
//	// 全ノードリスト取得
//	const std::vector<Node>& GetAllNodes() const { return  m_allNodes; }
//
//	// 指定名のノードを取得
//	const Node* GetNode(const std::string& name)
//	{
//		auto it = std::find_if(m_allNodes.begin(), m_allNodes.end(), [&name](Node& node)
//			{
//				return node.NodeName == name;
//			});
//		if (it != m_allNodes.end())
//			return &(*it);
//
//		return nullptr;
//	}
//
//	// 全ノード情報から、Transform配列を作成する(Transform内の親子構造も構築する)
//	const void CreateNodeTransforms(std::vector<KdTransform>& outTransforms) const
//	{
//		outTransforms.clear();
//		outTransforms.resize(m_allNodes.size());
//
//		// 全ノード
//		for (uint32_t i = 0; i < m_allNodes.size(); i++)
//		{
//			// Transformコピー
//			outTransforms[i].SetLocalMatrix(m_allNodes[i].Transform.GetLocalMatrix());
//
//			// 親Transformがある？
//			int parentIdx = m_allNodes[i].ParentIndex;
//			if (parentIdx >= 0)
//			{
//				// 親の設定
//				outTransforms[i].m_parent = &outTransforms[parentIdx];
//
//				// 親に対して、自分を子として設定
//				outTransforms[parentIdx].m_children.push_back(&outTransforms[i]);
//			}
//		}
//
//	}
//
//	// 全ノードリスト内の、ボーンのIndex集
//	const std::vector<uint32_t>& GetBoneIndices() const { return m_boneIndices; }
//	// 全ノードリスト内の、メッシュのIndex集
//	const std::vector<uint32_t>& GetMeshIndices() const { return m_meshIndices; }
//
//	// マテリアルリスト
//	const std::vector<std::shared_ptr<KdMaterial>>& GetMaterials() const { return m_materials; }
//
//	// アニメーションデータ
//	const std::vector<KdAnimationData>& GetAnimations()const {return m_animations;}
//
//	// 3Dモデルをロードする
//	bool Load(const std::string& filepath);
//
//
//	// 
//	KdModel() {}
//
//	~KdModel()
//	{
//		Release();
//	}
//
//	//-----------------------
//	// ノードデータ
//	//-----------------------
//	struct Node
//	{
//		// ノード名
//		std::string				NodeName;
//
//		// 自ノードIndex
//		int						NodeIndex = -1;
//		// ボーンノードである時のIndex。-1だとボーンではない
//		int						BoneIndex = -1;
//
//		// 子ノードIndexリスト
//		std::vector<int>		Children;
//		// 親ノードIndex
//		int						ParentIndex = -1;
//
//		// ローカル行列(親ノードからの相対行列)
//		KdTransform				Transform;
//
//		// ボーンオフセット行列
//		KdMatrix				BoneOffsetMatrix;
//
//		// メッシュ
//		std::shared_ptr<KdMesh>	Mesh;
//	};
//
//private:
//
//	void Release()
//	{
//		m_allNodes.clear();
//		m_materials.clear();
//		m_hasSkinMesh = false;
//	}
//
//	// アセットパス
//	std::string m_assetPath;
//
//	// 
//	bool m_hasSkinMesh = false;
//
//	// 全ノードリスト
//	std::vector<Node>					m_allNodes;
//
//	// 全ノードリスト内の、ボーンのIndex集
//	std::vector<uint32_t>						m_boneIndices;
//	// 全ノードリスト内の、メッシュのIndex集
//	std::vector<uint32_t>						m_meshIndices;
//
//	// マテリアルリスト
//	std::vector<std::shared_ptr<KdMaterial>>	m_materials;
//
//	// アニメーション情報
//	std::vector<KdAnimationData> m_animations;
//
//private:
//	// コピー禁止用
//	KdModel(const KdModel& src) = delete;
//	void operator=(const KdModel& src) = delete;
//};
