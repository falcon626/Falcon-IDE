#pragma once

class FlAudioManager : public BaseBasicResourceManager<FMOD::Sound>
{
public:
	FlAudioManager()  = default;
	~FlAudioManager() = default;

	const bool Load(const std::string& path) override;
private:
	inline auto FMODErrorCheck(const FMOD_RESULT result)
	{ 
		if (result != FMOD_OK) _ASSERT_EXPR(false, FMOD_ErrorString(result));
		return (result != FMOD_OK);
	}

	// <Custom Deleter:FMOD Sound>
	struct FMODSoundDeleter {
		void operator()(FMOD::Sound* sound) const {
			if (sound) sound->release();
		}
	};

	// <Custom Deleter:FMOD System>
	struct FMODSystemDeleter {
		void operator()(FMOD::System* system) const {
			if (system) system->release();
		}
	};

	std::unique_ptr<FMOD::System, FMODSystemDeleter> m_upSystem;
};