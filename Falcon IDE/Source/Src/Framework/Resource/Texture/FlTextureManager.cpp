#include "FlTextureManager.h"

const bool FlTextureManager::Load(const std::string& path)
{
	auto tex{ std::make_shared<Texture>() };
	if (!tex->Load(path)) return false;

	auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(path).value() };
	m_resources[guid] = tex;
	m_resources[guid]->SetCBVCount(Shader::Instance().GetCBVCount());

	FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Texture Loaded %s", path.c_str());
	std::make_unique<FlMetaFileManager>()->IncrementLoadFlag(path);

	return true;
}
