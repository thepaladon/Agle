#pragma once
#include <string>
#include <unordered_map>

namespace FMOD
{
	class Sound;
	class Channel;
} // namespace FMOD

namespace FMOD
{
	class System;
	class DSP;

	namespace Studio
	{
		class EventInstance;
		class Channel;
		class Sound;
		class Bank;
		class System;
		class Bus;
		class EventDescription;
	} // namespace Studio
} // namespace FMOD

namespace Ball
{
	enum class VolumeType : int
	{
		None = 0,
		Master = 1,
		Music = 2,
		SFX = 3
	};

	class AudioSystem
	{
	public:
		FMOD::Studio::System* GetStudioSystem() const { return m_System; }

		void PlatformInit();
		void Init();
		void Shutdown();
		void Update(float deltaTime);

		FMOD::Studio::Bank* LoadBank(const std::string& filepath);
		FMOD::Sound* PlayAudio(const std::string& filepath, unsigned int playMode);

		FMOD::Studio::EventDescription* CreateAudioEvent(const std::string& EventName);
		void PlayAudioEvent(const std::string& EventName);
		void StopAudioEvent(const std::string& EventName, const bool AllowFadeOut);

		/// <summary>
		/// Stop all sounds that are currently playing.
		/// </summary>
		void StopAllAudioEvents();

		// Returns a value that should be translated to an FMOD_STUDIO_PLAYBACK_STATE enum
		int GetEventPlaybackState(const std::string& EventName);

		bool GetIsEventPlaying(const std::string& eventName);

		const std::unordered_map<std::string, FMOD::Studio::EventInstance*> GetEventInstances()
		{
			return m_EventInstances;
		}

		void SetAllEffects();
		void ResetParameters();

		void SetVolume(float volume);
		void SetVolume(const std::string& EventName, float volume);
		void SetDirectoryVolume(const std::string& EventDirectory, float volume);
		float GetDirectoryVolume(const std::string& EventDirectory);

		void SetPitch(float pitch);
		void SetFrequency(float frequency);
		void SetPan(float pan);
		void SetFilters(float lowpass, float highpass);
		void SetReverb(float reverbLevel, float reverbDelay);
		void SetChorus(float chorusLevel, float chorusRate, float chorusDepth);
		void SetFlanger(float flangerMix, float flangerDepth);
		void SetEcho(float echoLevel, float echoDelay);

	private:
		friend class AudioParameter;
		float m_MasterVolume = 1.0f;
		float m_MusicVolume = 1.0f;
		float m_SFXVolume = 1.0f;

		float m_Pitch = 0;
		float m_Frequency = 0;
		float m_Pan = 0;

		// Filters
		float m_Lowpass = 1;
		float m_Highpass = 1;

		// Reverb
		float m_ReverbLevel = 0;
		float m_ReverbDelay = 0;

		// Chorus
		float m_ChorusLevel = 0;
		float m_ChorusRate = 0;
		float m_ChorusDepth = 0;

		// Flangers
		float m_FlangerDryMix = 0;
		float m_FlangerDepth = 0;

		// Echo
		float m_EchoLevel = 0;
		float m_EchoDelay = 0;

		bool m_Applyeffects = false;

		const float m_HertzMultiplier = 22000.f;

	private:
		FMOD::System* m_CoreSystem;
		FMOD::Studio::System* m_System;
		FMOD::Channel* m_Channel;
		// Create DSP effects and parameters for other effects
		FMOD::DSP* m_DSP = nullptr;

		// Map to store EventInstances for future reference
		std::unordered_map<std::string, FMOD::Studio::EventInstance*> m_EventInstances;

		const int m_MAXCHANNELS = 512;

		// Combines the master volume with the volume from specific event volume type, you can use volume to give
		// specific event custom volume
		float GetMixedVolume(const std::string eventName, float volume = 1.0f);

		VolumeType StringToVolumeType(const std::string& EventDirectory)
		{
			if (EventDirectory.find("Master") != std::string::npos)
				return VolumeType::Master;
			if (EventDirectory.find("Music") != std::string::npos)
				return VolumeType::Music;
			if (EventDirectory.find("SFX") != std::string::npos)
				return VolumeType::SFX;

			return VolumeType::None;
		}
	};
} // namespace Ball