#include "FlAudioManager.h"

FlAudioManager::FlAudioManager() noexcept
{
	FMOD::System* system{};
	if (FMODErrorCheck(FMOD::System_Create(&system))) return;

	m_upSystem.reset(system);
	if (FMODErrorCheck(m_upSystem->init(1024, FMOD_INIT_NORMAL, nullptr)))
		m_upSystem.reset();
}

FlAudioManager::~FlAudioManager()
{
	m_resources.clear();
	if (m_upSystem)
	{
		FMODErrorCheck(m_upSystem->close());
		m_upSystem.reset();
	}
}

const bool FlAudioManager::Load(const std::string& path)
{
	if (!m_upSystem) return false;

	auto rawSound{ static_cast<FMOD::Sound*>(nullptr) };

	if (FMODErrorCheck(m_upSystem->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &rawSound))) return false;

	auto spSound{ std::shared_ptr<FMOD::Sound>{rawSound, FMODSoundDeleter{}} };

	auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(path) };
	if (!guid) return false;
	m_resources[*guid] = std::move(spSound);

	FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Sound Loaded %s", path.c_str());
	FlResourceAdministrator::Instance().GetMetaFileManager()->IncrementLoadFlag(path);

	return true;
}
