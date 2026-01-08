#pragma once

// <Shader:描画関連>
#include "Shader/ShaderManager.h"

// <Texture:画像関連>
#include "Texture/FlTextureManager.h"

// <Model:モデル関連>
#include "Model/ModelManager.h"

// <Audio:音声関連>
#include "Audio/FlAudioManager.h"

// <Binary:バイナリ関連>
#include "Binary/FlBinaryManager.hpp"

// <Meta:メタファイル関連>
#include "Meta/FlMetaFileManager.h"

/// <summary> =Singleton= </summary>
class FlResourceAdministrator
{
public:
	void Load(const std::initializer_list<std::string>& assetsPaths) noexcept;

	template<typename T>
	const std::shared_ptr<T> Get(const std::string& path)
	{
		auto fullPath{ "Assets/" + path };
		auto optGuid{ m_meta->FindGuidByAsset(fullPath) };
		if (!optGuid.has_value()) return nullptr;

		auto guid{ optGuid.value() };

		if constexpr (std::is_same_v<T, ComPtr<ID3DBlob>>)
			return m_shader->Get(fullPath, guid);

		else if constexpr (std::is_same_v<T, Texture>)
			return m_texture->Get(fullPath, guid);

		else if constexpr (std::is_same_v<T, ModelData>)
			return m_model->Get(fullPath, guid);

		else if constexpr (std::is_same_v<T, FMOD::Sound>)
			return m_audio->Get(fullPath, guid);

		else
		{
			// 対応していない型が指定された場合にコンパイルエラーを出す
			static_assert(std::is_void_v<T>, "FlResourceAdministrator::Get() - Unsupported resource type requested!");
			return nullptr;
		}
	}

	template<typename T>
	const std::shared_ptr<T> GetByGuid(const std::string& guid)
	{
		auto fullPath{ m_meta->FindAssetByGuid(guid) };
		if (!fullPath.has_value()) return nullptr;

		if constexpr (std::is_same_v<T, ComPtr<ID3DBlob>>)
			return m_shader->Get(fullPath.value(), guid);

		else if constexpr (std::is_same_v<T, Texture>)
			return m_texture->Get(fullPath.value(), guid);

		else if constexpr (std::is_same_v<T, ModelData>)
			return m_model->Get(fullPath.value(), guid);

		else if constexpr (std::is_same_v<T, FMOD::Sound>)
			return m_audio->Get(fullPath.value(), guid);

		else
		{
			// 対応していない型が指定された場合にコンパイルエラーを出す
			static_assert(std::is_void_v<T>, "FlResourceAdministrator::GetByGuid() - Unsupported resource type requested!");
			return nullptr;
		}
	}

	/// <summary>
	/// #Getter メタファイルマネージャーの参照を返す
	/// </summary>
	/// <returns>ユニークなメタファイルマネージャークラスの参照インスタンス</returns>
	const auto& GetMetaFileManager() const noexcept { return m_meta; }

	/// <summary>
	/// #警告 すべてのアセットキャッシュをクリアします。
	/// </summary>
	void AllAssetsCacheClear() noexcept;

	/// <summary>
	/// ResourceAdministrator の唯一のインスタンスへの参照を返します（シングルトンパターン）。
	/// </summary>
	/// <returns>ResourceAdministrator の静的インスタンスへの参照。</returns>
	inline static auto& Instance() noexcept
	{
		static auto instance{ FlResourceAdministrator{} };
		return instance;
	}

private:
	FlResourceAdministrator()
		: m_shader  { std::make_unique<ShaderManager>() }
		, m_texture { std::make_unique<FlTextureManager>() }
		, m_model   { std::make_unique<ModelManager>() }
		, m_audio   { std::make_unique<FlAudioManager>() }
		, m_binary  { std::make_unique<FlBinaryManager>() }
		, m_meta    { std::make_unique<FlMetaFileManager>() }
		, m_upFPM   { std::make_unique<FlFilePathManager>() }
	{
		m_meta->StartMonitoring("Assets");
	}

	~FlResourceAdministrator() { AllAssetsCacheClear(); };

	std::unique_ptr<ShaderManager>    m_shader;
	std::unique_ptr<FlTextureManager> m_texture;
	std::unique_ptr<ModelManager>     m_model;
	std::unique_ptr<FlAudioManager>   m_audio;

	std::unique_ptr<FlBinaryManager>  m_binary;

	std::unique_ptr<FlMetaFileManager> m_meta;

	std::unique_ptr<FlFilePathManager> m_upFPM;

	const std::string BaseAssetsPath{ "Assets" };
};