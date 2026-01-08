#include "FlAudioManagerOld.h"

FlAudioManager::FlAudioManager() noexcept
{
	{ // <Initialize:FMOD::System>
		auto rawSystem{ static_cast<FMOD::System*>(nullptr) };

		FMODErrorCheck(FMOD::System_Create(&rawSystem));
		m_upSystem.reset(rawSystem);
		FMODErrorCheck(m_upSystem->init(static_cast<int32_t>(Def::BitMaskPos10), FMOD_INIT_NORMAL, nullptr));
	}

	{ // <Initialize:FMOD::ChannelGroup>
		auto rawChannelGroup{ static_cast<FMOD::ChannelGroup*>(nullptr) };

		FMODErrorCheck(m_upSystem->createChannelGroup("MasterGroup", &rawChannelGroup));
		m_upChannelGroup.reset(rawChannelGroup);
	}
}

const std::shared_ptr<FMOD::Sound> FlAudioManager::LoadSound(const std::string& filePath)
{
	// <Quick Return:std::shared_ptr<FMOD::Sound>>
	if (m_soundController.find(filePath) != m_soundController.end()) return m_soundController[filePath].first;

	auto rawSound{ static_cast<FMOD::Sound*>(nullptr) };
	FMODErrorCheck(m_upSystem->createSound(filePath.c_str(), FMOD_DEFAULT, nullptr, &rawSound));

	auto upNewSound{ std::unique_ptr<FMOD::Sound, FMODSoundDeleter>{rawSound} };

	auto spSound{ std::shared_ptr<FMOD::Sound>(upNewSound.release(), FMODSoundDeleter()) };
	m_soundController[filePath] = { spSound, nullptr }; 

	return spSound;
}

const std::shared_ptr<FMOD::Sound> FlAudioManager::LoadStreamSound(const std::string& filePath)
{
	if (m_soundController.find(filePath) != m_soundController.end()) return m_soundController[filePath].first;

	auto rawSound{ static_cast<FMOD::Sound*>(nullptr) };

	FMODErrorCheck(m_upSystem->createSound(filePath.c_str(), FMOD_CREATESTREAM | FMOD_DEFAULT, nullptr, &rawSound));

	auto upNewSound{ std::unique_ptr<FMOD::Sound, FMODSoundDeleter>{rawSound} };

	auto spSound{ std::shared_ptr<FMOD::Sound>(upNewSound.release(), FMODSoundDeleter()) };
	m_soundController[filePath] = { spSound, nullptr };

	return spSound;
}

const std::shared_ptr<FMOD::Sound> FlAudioManager::LoadSoundMultiple(const std::string& filePath,const FMOD_MODE mode)
{
	auto it{ m_soundControllerMultiple.find(filePath) };

	if (it != m_soundControllerMultiple.end())
		return it->second.sound_;

	auto rawSound{ static_cast<FMOD::Sound*>(nullptr) };
	FMODErrorCheck(m_upSystem->createSound(filePath.c_str(), mode, nullptr, &rawSound));

	auto spSound{ std::shared_ptr<FMOD::Sound>(rawSound, FMODSoundDeleter()) };
	m_soundControllerMultiple[filePath] = SoundEntry{ spSound, {} };

	return spSound;
}

void FlAudioManager::Play()
{
	auto it{ m_soundController.begin() };

	Stop();

	while (it != m_soundController.end())
	{
		if (!LoadSound(it->first)) continue;

		auto& soundChannelPair{ m_soundController[it->first] };
		auto& spSound         { soundChannelPair.first };
		auto& upChannel       { soundChannelPair.second };

		if (spSound)
		{
			auto mode{ FMOD_MODE{} };
			FMODErrorCheck(spSound->getMode(&mode));

			auto rawChannel{ static_cast<FMOD::Channel*>(nullptr) };
			m_upSystem->playSound(spSound.get(), m_upChannelGroup.get(), false, &rawChannel);

			if (rawChannel) rawChannel->setMode(mode);

			upChannel.reset(rawChannel);
		}

		++it;
	}
}

void FlAudioManager::Play(const std::string& filePath, const bool isLoop)
{
	// <Quick Return:オーディオファイルがないもしくは再生中であるなら>
	if (!LoadSound(filePath) || IsPlay(filePath)) return;

	auto& soundChannelPair{ m_soundController[filePath] };
	auto& spSound         { soundChannelPair.first };
	auto& upChannel       { soundChannelPair.second };

	auto mode{ FMOD_MODE{} };
	FMODErrorCheck(spSound->getMode(&mode));

	if (spSound)
	{
		auto rawChannel{ static_cast<FMOD::Channel*>(nullptr) };
		m_upSystem->playSound(spSound.get(), m_upChannelGroup.get(), false, &rawChannel);

		if (rawChannel && isLoop) rawChannel->setMode(mode | FMOD_LOOP_NORMAL);
		else if (rawChannel) rawChannel->setMode(~(mode & FMOD_LOOP_NORMAL));
		else _ASSERT_EXPR(false, "Channel Is Empty");

		upChannel.reset(rawChannel);
	}
}

void FlAudioManager::PlayMultiple(const std::string& filePath, bool isLoop, bool is3D)
{
	auto spSound{ LoadSoundMultiple(filePath, (is3D ? FMOD_3D : FMOD_DEFAULT)) };
	if (!spSound) return;

	auto rawChannel{ static_cast<FMOD::Channel*>(nullptr) };
	FMODErrorCheck(m_upSystem->playSound(spSound.get(), m_upChannelGroup.get(), false, &rawChannel));

	auto mode{ FMOD_MODE{} };
	FMODErrorCheck(spSound->getMode(&mode));
	if (isLoop) mode |= FMOD_LOOP_NORMAL;
	else mode &= ~FMOD_LOOP_NORMAL;
	if (is3D) mode |= FMOD_3D;
	FMODErrorCheck(rawChannel->setMode(mode));

	m_soundControllerMultiple[filePath].channels_.emplace_back(rawChannel);
}

void FlAudioManager::Play3D()
{
	Stop();

	auto it{ m_soundController.begin() };

	while (it != m_soundController.end())
	{
		if (!LoadSound(it->first)) continue;

		auto& soundChannelPair	{ m_soundController[it->first] };
		auto& spSound			{ soundChannelPair.first };
		auto& upChannel			{ soundChannelPair.second };



		if (spSound)
		{
			auto mode{ FMOD_MODE{} };
			spSound->getMode(&mode);

			auto rawChannel{ static_cast<FMOD::Channel*>(nullptr) };
			FMODErrorCheck(m_upSystem->playSound(spSound.get(), m_upChannelGroup.get(), false, &rawChannel));

			if (rawChannel) FMODErrorCheck(rawChannel->setMode(mode | FMOD_3D));

			upChannel.reset(rawChannel);
		}

		++it;
	}
}

void FlAudioManager::Play3D(const std::string& filePath, const bool isLoop)
{
	// <Quick Return:オーディオファイルがないもしくは再生中であるなら>
	if (!LoadSound(filePath) || IsPlay(filePath)) return;

	auto& soundChannelPair	{ m_soundController[filePath] };
	auto& spSound			{ soundChannelPair.first };
	auto& upChannel			{ soundChannelPair.second };

	if (spSound)
	{
		auto mode{ FMOD_MODE{} };
		FMODErrorCheck(spSound->getMode(&mode));

		auto rawChannel		{ static_cast<FMOD::Channel*>(nullptr) };
		FMODErrorCheck(m_upSystem->playSound(spSound.get(), m_upChannelGroup.get(), false, &rawChannel));

		if (rawChannel && isLoop) FMODErrorCheck(rawChannel->setMode(mode | FMOD_LOOP_NORMAL | FMOD_3D));
		else if (rawChannel) FMODErrorCheck(rawChannel->setMode(~(mode & FMOD_LOOP_NORMAL) | FMOD_3D));

		upChannel.reset(rawChannel);
	}
}

const bool FlAudioManager::IsPlay()
{
	auto is{ false };
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->isPlaying(&is));
	return is;
}

const bool FlAudioManager::IsPlay(const std::string& filePath)
{
	auto it{ m_soundController.find(filePath) };
	auto is{ false };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->isPlaying(&is));
	return is;
}

const bool FlAudioManager::IsPlayMultiple(const std::string& filePath)
{
	auto it{ m_soundControllerMultiple.find(filePath) };
	if (it == m_soundControllerMultiple.end()) return false;
	for (auto& channel : it->second.channels_) {
		auto isPlaying{ false };
		if(channel) FMODErrorCheck(channel->isPlaying(&isPlaying));
		if (isPlaying) return true;
	}
	return false;
}

void FlAudioManager::Stop()
{
	if (!m_soundControllerMultiple.empty()) {
		for (auto& kv : m_soundControllerMultiple) {
			for (auto& channel : kv.second.channels_)
				if (channel) FMODErrorCheck(channel->stop());
			kv.second.channels_.clear();
		}
	}
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->stop());
}

void FlAudioManager::Stop(const std::string& filePath)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->stop());
}

void FlAudioManager::StopMultiple(const std::string& filePath)
{
	auto it = m_soundControllerMultiple.find(filePath);
	if (it != m_soundControllerMultiple.end()) {
		for (auto& channel : it->second.channels_) {
			if (channel) FMODErrorCheck(channel->stop());
		}
		it->second.channels_.clear();
	}
}

void FlAudioManager::Set3DSoundSettings(const Math::Vector3& position, const Math::Vector3& velocity)
{
	auto pos{ SimpleMathToFmodVec(position) };
	auto vel{ SimpleMathToFmodVec(velocity) };
	if (m_upChannelGroup) m_upChannelGroup->set3DAttributes(&pos, &vel);
}

void FlAudioManager::Set3DSoundSettings(const std::string& filePath, const Math::Vector3& position, const Math::Vector3& velocity)
{
	auto pos{ SimpleMathToFmodVec(position) };
	auto vel{ SimpleMathToFmodVec(velocity) };
	auto it { m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->set3DAttributes(&pos, &vel));
}

void FlAudioManager::Set3DSoundSettingsMultiple(const std::string& filePath, const Math::Vector3& position, const Math::Vector3& velocity)
{
	auto pos{ SimpleMathToFmodVec(position) };
	auto vel{ SimpleMathToFmodVec(velocity) };
	auto it{ m_soundControllerMultiple.find(filePath) };
	if (it != m_soundControllerMultiple.end())
		for (auto& channel : it->second.channels_)
			if (channel) FMODErrorCheck(channel->set3DAttributes(&pos, &vel));
}

void FlAudioManager::Set3DListenerAttributes(const Math::Vector3& listenerPosition, const Math::Vector3& listenerVelocity, const Math::Vector3& forward, const Math::Vector3& up)
{
	auto pos  { SimpleMathToFmodVec(listenerPosition) };
	auto vel  { SimpleMathToFmodVec(listenerVelocity) };
	auto fwd  { SimpleMathToFmodVec(forward) };
	auto upVec{ SimpleMathToFmodVec(up) };

	FMODErrorCheck(m_upSystem->set3DListenerAttributes(Def::IntZero, &pos, &vel, &fwd, &upVec));
}

void FlAudioManager::SetVolume(float volume)
{
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->setVolume(volume));
}

void FlAudioManager::SetVolume(const std::string& filePath, float volume)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->setVolume(volume));
}

void FlAudioManager::SetVolumeMultiple(const std::string& filePath, float volume)
{
	auto it{ m_soundControllerMultiple.find(filePath) };
	if (it != m_soundControllerMultiple.end()) {
		for (auto& channel : it->second.channels_)
			if (channel) FMODErrorCheck(channel->setVolume(volume));
	}
}

void FlAudioManager::Mute(const bool isMute)
{
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->setMute(isMute));
}

void FlAudioManager::Mute(const std::string& filePath, const bool isMute)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->setMute(isMute));
}

void FlAudioManager::SetPitch(float pitch)
{
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->setPitch(pitch));
}

void FlAudioManager::SetPitch(const std::string& filePath, float pitch)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->setPitch(pitch));
}

void FlAudioManager::SetReverb(float reverbLevel)
{
	auto rawReverbDSP	{ static_cast<FMOD::DSP*>(nullptr) };

	FMODErrorCheck(m_upSystem->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &rawReverbDSP));
	FMODErrorCheck(rawReverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, reverbLevel));

	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->addDSP(Def::IntZero, rawReverbDSP));

	FMODErrorCheck(rawReverbDSP->release());
}

void FlAudioManager::SetReverb(const std::string& filePath, float reverbLevel)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second)
	{
		auto rawReverbDSP{ static_cast<FMOD::DSP*>(nullptr) };
		FMODErrorCheck(m_upSystem->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &rawReverbDSP));
		FMODErrorCheck(rawReverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, reverbLevel));
		FMODErrorCheck(it->second.second->addDSP(Def::IntZero, rawReverbDSP));
		FMODErrorCheck(rawReverbDSP->release());
	}
}

void FlAudioManager::SetMode(const FMOD_MODE mode)
{
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->setMode(mode));
}

void FlAudioManager::SetMode(const std::string& filePath, const FMOD_MODE mode)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->setMode(mode));
}

void FlAudioManager::Pause()
{
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->setPaused(true));
}

void FlAudioManager::Pause(const std::string& filePath)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->setPaused(true));
}

void FlAudioManager::Resume()
{
	if (m_upChannelGroup) FMODErrorCheck(m_upChannelGroup->setPaused(false));
}

void FlAudioManager::Resume(const std::string& filePath)
{
	auto it{ m_soundController.find(filePath) };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->setPaused(false));
}

const bool FlAudioManager::IsPause()
{
	auto is{ false };
	if (m_upChannelGroup) m_upChannelGroup->getPaused(&is);
	return is;
}

const bool FlAudioManager::IsPause(const std::string& filePath)
{
	auto it{ m_soundController.find(filePath) };
	auto is{ false };
	if (it != m_soundController.end() && it->second.second) FMODErrorCheck(it->second.second->getPaused(&is));
	return is;
}

void FlAudioManager::Update()
{
	m_upSystem->update();
}

FMOD::DSP* FlAudioManager::CreateLowCutFilter(FMOD::System* system, float cutoffHz, float resonance) noexcept
{
	auto dsp{ static_cast<FMOD::DSP*>(nullptr) };

	// ハイパスフィルターDSPを作成
	FMODErrorCheck(system->createDSPByType(FMOD_DSP_TYPE_HIGHPASS, &dsp));

	// パラメータ設定
	FMODErrorCheck(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_CUTOFF, cutoffHz));
	FMODErrorCheck(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_RESONANCE, resonance));

	return dsp;
}

FMOD::DSP* FlAudioManager::CreateHighCutFilter(FMOD::System* system, float cutoffHz, float resonance) noexcept
{
	auto dsp{ static_cast<FMOD::DSP*>(nullptr) };

	// ローパスフィルターDSPを作成
	FMODErrorCheck(system->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &dsp));

	// パラメータ設定
	FMODErrorCheck(dsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoffHz));
	FMODErrorCheck(dsp->setParameterFloat(FMOD_DSP_LOWPASS_RESONANCE, resonance));

	return dsp;
}
