#include "FlResourceAdministrator.h"

void FlResourceAdministrator::Load(const std::initializer_list<std::string>& assetsPaths) noexcept
{
	for (auto& assetsPath : assetsPaths)
	{
		auto ext{ Str::FileExtensionSearcher(assetsPath) };
		const auto optGuid = m_meta->FindGuidByAsset(assetsPath);
		if (!optGuid) continue;
		const auto& guid = *optGuid;

		if (ext.empty()) continue;

		if (ext == "gltf" || ext == "fbx")
		{
			if (!m_model->Get(assetsPath, guid))
				FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Error: Model load %s", assetsPath.c_str());
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
