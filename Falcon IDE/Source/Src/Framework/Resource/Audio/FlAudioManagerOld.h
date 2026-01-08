#pragma once

/// <summary> =Flyweight= </summary>
class FlAudioManager 
{
public:

	FlAudioManager() noexcept;
	~FlAudioManager() = default;

	const std::shared_ptr<FMOD::Sound> LoadSound(const std::string& filePath);
	const std::shared_ptr<FMOD::Sound> LoadStreamSound(const std::string& filePath);

	const std::shared_ptr<FMOD::Sound> LoadSoundMultiple(const std::string& filePath, const FMOD_MODE mode);

	void Play();
	void Play(const std::string& filePath, const bool isLoop = false);

	void PlayMultiple(const std::string& filePath, bool isLoop = false , bool is3D = false);

	void Play3D();
	void Play3D(const std::string& filePath, const bool isLoop = false);

	const bool IsPlay();
	const bool IsPlay(const std::string& filePath);

	const bool IsPlayMultiple(const std::string& filePath);

	void Stop();
	void Stop(const std::string& filePath);

	void StopMultiple(const std::string& filePath);

	void Set3DSoundSettings(const Math::Vector3& position, const Math::Vector3& velocity);
	void Set3DSoundSettings(const std::string& filePath, const Math::Vector3& position, const Math::Vector3& velocity);
	
	void Set3DSoundSettingsMultiple(const std::string& filePath, const Math::Vector3& position, const Math::Vector3& velocity);

	void Set3DListenerAttributes(const Math::Vector3& listenerPosition, const Math::Vector3& listenerVelocity, const Math::Vector3& forward, const Math::Vector3& up);

	void SetVolume(float volume);
	void SetVolume(const std::string& filePath, float volume);

	void SetVolumeMultiple(const std::string& filePath, float volume);

	void Mute(const bool isMute = true);
	void Mute(const std::string& filePath, const bool isMute = true);

	void SetPitch(float pitch);
	void SetPitch(const std::string& filePath, float pitch);

	void SetReverb(float reverbLevel);
	void SetReverb(const std::string& filePath, float reverbLevel);

	void SetMode(const FMOD_MODE mode);
	void SetMode(const std::string& filePath, const FMOD_MODE mode);

	void Pause();
	void Pause(const std::string& filePath);

	void Resume();
	void Resume(const std::string& filePath);

	const bool IsPause();
	const bool IsPause(const std::string& filePath);

	void Update();

	inline auto ClearCache() noexcept { m_soundController.clear(); }

	FlAudioManager(const FlAudioManager&) = delete;
	FlAudioManager& operator=(const FlAudioManager&) = delete;
private:
	/// <summary>
	/// FMOD の関数呼び出し結果をチェックし、エラーがあればアサートを発生させます。
	/// </summary>
	/// <param name="result">FMOD 関数の戻り値（FMOD_RESULT 型）。</param>
	/// <returns>なし（この関数は値を返しません）。</returns>
	inline auto FMODErrorCheck(const FMOD_RESULT result)
		{ if (result != FMOD_OK) _ASSERT_EXPR(false, FMOD_ErrorString(result)); }
	
	//↓--Vector3 Converter--↓//
	inline const auto FmodVecToSimpleMath(const FMOD_VECTOR& vec) noexcept { Math::Vector3 resultVec{ vec.x,vec.y,vec.z }; return resultVec; }
	inline const auto SimpleMathToFmodVec(const Math::Vector3& vec) noexcept { FMOD_VECTOR resultVec{ vec.x,vec.y,vec.z }; return resultVec; }
	//↑--Vector3 Converter--↑//

	FMOD::DSP* CreateLowCutFilter (FMOD::System* system, float cutoffHz, float resonance = Def::FloatOne) noexcept;
	FMOD::DSP* CreateHighCutFilter(FMOD::System* system, float cutoffHz, float resonance = Def::FloatOne) noexcept;

	// <Custom Deleter:FMOD System>
	struct FMODSystemDeleter {
		void operator()(FMOD::System* system) const {
			if (system) system->release();
		}
	};

	// <Custom Deleter:FMOD Sound>
	struct FMODSoundDeleter {
		void operator()(FMOD::Sound* sound) const {
			if (sound) sound->release();
		}
	};

	// <Custom Deleter:FMOD ChannelGroup>
	struct FMODChannelGroupDeleter {
		void operator()(FMOD::ChannelGroup* channelGroup) const {
			if (channelGroup)
			{
				channelGroup->stop();
				channelGroup->release();
			}
		}
	};

	// <Custom Deleter:FMOD Channel>
	struct FMODChannelDeleter {
		void operator()(FMOD::Channel* channel) const {
			if (channel) channel->stop();
		}
	};

	std::unique_ptr<FMOD::System, FMODSystemDeleter> m_upSystem;
	std::unique_ptr<FMOD::ChannelGroup, FMODChannelGroupDeleter> m_upChannelGroup;

	std::unordered_map<
		std::string, std::pair<
		std::shared_ptr<FMOD::Sound>, std::unique_ptr<FMOD::Channel, FMODChannelDeleter>
		>
	> m_soundController;

	struct SoundEntry {
		std::shared_ptr<FMOD::Sound> sound_;
		std::list<std::unique_ptr<FMOD::Channel, FlAudioManager::FMODChannelDeleter>> channels_;
	};
	std::unordered_map<std::string, SoundEntry> m_soundControllerMultiple; // 多重再生用
};