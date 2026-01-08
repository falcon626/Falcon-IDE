#include "FlAudioManager.h"

const bool FlAudioManager::Load(const std::string& path)
{
	auto rawSound{ static_cast<FMOD::Sound*>(nullptr) };

	if (FMODErrorCheck(m_upSystem->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &rawSound))) return false;

	auto spSound{ std::shared_ptr<FMOD::Sound>{(std::unique_ptr<FMOD::Sound, FMODSoundDeleter>{rawSound}).release(), FMODSoundDeleter()} };

	auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(path).value() };
	m_resources[guid] = spSound;

	FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Sound Loaded %s", path.c_str());
	std::make_unique<FlMetaFileManager>()->IncrementLoadFlag(path);

	return true;
}