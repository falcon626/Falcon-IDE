#include "FlResourceAdministrator.h"

void FlResourceAdministrator::Load(const std::initializer_list<std::string>& assetsPaths) noexcept
{
	for (auto& assetsPath : assetsPaths)
	{
		auto ext{ Str::FileExtensionSearcher(assetsPath) };
		auto guid{ m_meta->FindGuidByAsset(assetsPath).value() };

		if (ext.empty()) continue;

		if (ext == "gltf" || ext == "fbx")
		{
			if (!m_model->Get(assetsPath, guid))
				FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Model load %s", assetsPath);
		}
	}
}

void FlResourceAdministrator::AllAssetsCacheClear() noexcept
{
	m_shader ->Clear();
	m_texture->Clear();
	m_model  ->Clear();
	m_audio  ->Clear();
}
