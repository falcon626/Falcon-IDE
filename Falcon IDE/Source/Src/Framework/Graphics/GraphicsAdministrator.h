#pragma once

class GraphicsDevice;

#include "GraphicsDevice.h"

// <Heap:ヒープ>
#include "Heap/Heap.h"
#include "Heap/RTVHeap/RTVHeap.h"
#include "Heap/CBVSRVUAVHeap/CBVSRVUAVHeap.h"
#include "Heap/DSVHeap/DSVHeap.h"

// <Buffer:バッファ>
#include "Buffer/Buffer.h"

// 定数バッファのアロケーター
#include "Buffer/CBufferAllocater/CBufferAllocater.h"

// 定数バッファデータ
#include "Buffer/CBufferAllocater/CBufferData/CBufferData.h"

// 頂点バッファ
#include "Buffer/VBufferAllocator/VBufferAllocator.h"

// テクスチャ
#include "Buffer/Texture/Texture.h"
#include "Buffer/Texture/FlTexture.h"

// デプスステンシル
#include "Buffer/DepthStencil/DepthStencil.h"

// メッシュ
#include "Mesh/Mesh.h"

// モデル
#include "Model/Model.h"

// アニメーション
#include "Animation/Animation.h"

// <Shader:シェーダ>
#include "Shader/Shader.h"


class GraphicsAdministrator
{
public:

	auto SetGraphicsDevice(GraphicsDevice* dev) noexcept { m_pGraphicsDev = dev; }
	const auto& GetGraphicsDevice() const noexcept { return m_pGraphicsDev; }

	auto SetShader(Shader* shad)noexcept { m_pShader = shad; }
	const auto& GetShader() const noexcept { return m_pShader; }

	auto DrawModel(ModelData& modelData, const Math::Matrix& worldMatrix)
	{
		if (m_pShader)
			m_pShader->DrawModel(modelData, worldMatrix);
	}

	static auto& Instance() noexcept
	{
		static auto instance{ GraphicsAdministrator{} };
		return instance;
	}
private:

	GraphicsAdministrator()  = default;
	~GraphicsAdministrator() { m_pGraphicsDev = nullptr; m_pShader = nullptr; }

	GraphicsAdministrator(const GraphicsAdministrator&) = delete;            // コピーコンストラクタ削除
	GraphicsAdministrator& operator=(const GraphicsAdministrator&) = delete; // 代入演算子削除
	GraphicsAdministrator(GraphicsAdministrator&&) = delete;                 // ムーブコンストラクタ削除
	GraphicsAdministrator& operator=(GraphicsAdministrator&&) = delete;      // ムーブ代入演算子削除

	GraphicsDevice* m_pGraphicsDev{ nullptr };
	Shader* m_pShader{ nullptr };
};