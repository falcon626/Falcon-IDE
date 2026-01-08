#include "ModelManager.h"

const bool ModelManager::Load(const std::string& path)
{
	auto modelData{ std::make_shared<ModelData>() };

	if (!modelData->Load(path))
	{
		//assert(false && "ƒ‚ƒfƒ‹‚Ìƒ[ƒh‚ÉŽ¸”s");
		return false;
	}
	
	auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(path).value() };
	m_resources[guid] = modelData;

	FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Model Loaded %s", path.c_str());
	std::make_unique<FlMetaFileManager>()->IncrementLoadFlag(path);

	return true;
}
