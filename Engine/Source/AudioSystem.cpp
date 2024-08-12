#include "AudioSystem.h"

#include <../External/FMOD/inc/fmod_errors.h>
#include <../External/FMOD/inc/fmod.hpp>
#include <../External/FMOD/inc/fmod_studio.hpp>
#include <../External/FMOD/inc/fmod_common.h>
#include "FileIO.h"
#include "Log.h"
#include "Engine.h"
#include "Utilities/LaunchParameters.h"

using namespace Ball;

void AudioSystem::Init()
{
	PlatformInit();

	// Create the Studio system object
	[[maybe_unused]] FMOD_RESULT result = FMOD::Studio::System::create(&m_System);

	[[maybe_unused]] const char* errorMessage = FMOD_ErrorString(result);
	ASSERT_MSG(LOG_LOGGING, result == FMOD_OK, "Failed to create FMOD Studio system: %s", errorMessage);

	// Initialize the Studio system
	result = m_System->initialize(m_MAXCHANNELS, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_NORMAL, nullptr);

	errorMessage = FMOD_ErrorString(result);
	if (result == FMOD_ERR_OUTPUT_INIT)
	{
		m_System->release();

		FMOD::Studio::System::create(&m_System);
		m_System->getCoreSystem(&m_CoreSystem);
		m_CoreSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
		result = m_System->initialize(m_MAXCHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_LOGGING, result == FMOD_OK, "Failed to initialize FMOD Studio system: %s", errorMessage);
	}
	ASSERT_MSG(LOG_LOGGING, result == FMOD_OK, "Failed to initialize FMOD Studio system: %s", errorMessage);

	// Retrieve the low-level system from the Studio system
	result = m_System->getCoreSystem(&m_CoreSystem);

	errorMessage = FMOD_ErrorString(result);
	ASSERT_MSG(LOG_LOGGING, result == FMOD_OK, "Failed to create Core FMOD Studio system: %s", errorMessage);

	// set starting volume with launch parameter
	SetVolume(LaunchParameters::GetFloat("Volume", 1.f));
}

void AudioSystem::Shutdown()
{
	m_System->release();
	m_CoreSystem->release();
}

void AudioSystem::Update(float deltaTime)
{
	if (m_Applyeffects)
		SetAllEffects();
	m_System->update();
}

FMOD::Studio::Bank* AudioSystem::LoadBank(const std::string& filepath)
{
	FMOD::Studio::Bank* bank = nullptr;

	std::string tempPath;
	if (filepath.find(FileIO::GetPath(FileIO::DirectoryType::Audio)) == std::string::npos)
		tempPath = FileIO::GetPath(FileIO::DirectoryType::Audio) + filepath;
	else
		tempPath = filepath;

	[[maybe_unused]] FMOD_RESULT result = m_System->loadBankFile(tempPath.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);

	[[maybe_unused]] const char* errorMessage = FMOD_ErrorString(result);
	ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to load bank: %s", errorMessage);

	return bank;
}

FMOD::Sound* AudioSystem::PlayAudio(const std::string& filepath, FMOD_MODE playMode)
{
	FMOD::Sound* sound = nullptr;

	std::string tempPath;
	if (filepath.find(FileIO::GetPath(FileIO::DirectoryType::Audio)) == std::string::npos)
		tempPath = FileIO::GetPath(FileIO::DirectoryType::Audio) + filepath;
	else
		tempPath = filepath;

	[[maybe_unused]] FMOD_RESULT result = m_CoreSystem->createSound(tempPath.c_str(), playMode, nullptr, &sound);

	[[maybe_unused]] const char* errorMessage = FMOD_ErrorString(result);
	ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to create Sound: %s", errorMessage);

	result = m_CoreSystem->playSound(sound, nullptr, false, &m_Channel);

	errorMessage = FMOD_ErrorString(result);
	ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to play Sound: %s", errorMessage);

	return sound;
}

FMOD::Studio::EventDescription* AudioSystem::CreateAudioEvent(const std::string& eventName)
{
	// Construct the event path based on the provided event name
	std::string eventPath = "event:/"; // Assuming events are in the "event:/" category
	eventPath += eventName;

	// Get the EventDescription for the specified event
	FMOD::Studio::EventDescription* eventDescription = nullptr;
	FMOD_RESULT result = m_System->getEvent(eventPath.c_str(), &eventDescription);

	const char* errorMessage = FMOD_ErrorString(result);
	ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to create FMOD Event Description: %s", errorMessage);

	return eventDescription;
}

void AudioSystem::PlayAudioEvent(const std::string& eventName)
{
	FMOD::Studio::EventDescription* eventDescription = CreateAudioEvent(eventName);
	if (eventDescription)
	{
		FMOD::Studio::EventInstance* eventInstance = nullptr;
		FMOD_RESULT result = eventDescription->createInstance(&eventInstance);
		if (result != FMOD_OK)
		{
			const char* errorMessage = FMOD_ErrorString(result);
			ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to create FMOD Event Instance: %s", errorMessage);
		}

		// Store the event instance for future reference
		m_EventInstances[eventName] = eventInstance;

		// Set parameters (if needed)
		[[maybe_unused]] float parameterValue = 0.5f;
		// result = eventInstance->setParameterValue("ParameterName", parameterValue);

		// Start playing the instance
		result = eventInstance->start();
		if (result != FMOD_OK)
		{
			const char* errorMessage = FMOD_ErrorString(result);
			ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to start FMOD Event Instance: %s", errorMessage);
		}

		float MixedVolume = GetMixedVolume(eventName);
		eventInstance->setVolume(MixedVolume);

		if (result != FMOD_OK)
		{
			// Release the event instance when done.
			eventInstance->release();
		}
		else
			ERROR(LOG_AUDIO, "Result failed: Cannot release audio.");
	}
}

int AudioSystem::GetEventPlaybackState(const std::string& eventName)
{
	FMOD_STUDIO_PLAYBACK_STATE playbackState;
	auto eventInstanceIterator = m_EventInstances.find(eventName);

	if (eventInstanceIterator != m_EventInstances.end())
	{
		// Check if the event is playing.
		FMOD::Studio::EventInstance* eventInstance = eventInstanceIterator->second;
		FMOD_RESULT result = eventInstance->getPlaybackState(&playbackState);
		if (result != FMOD_OK)
		{
			const char* error = FMOD_ErrorString(result);
			ASSERT_MSG(
				LOG_AUDIO, result == FMOD_OK, "Failed to check playback state of FMOD Event Instance: %s", error);
		}
	}
	else
		playbackState = FMOD_STUDIO_PLAYBACK_STOPPED;
	return playbackState;
}

bool Ball::AudioSystem::GetIsEventPlaying(const std::string& eventName)
{
	FMOD_STUDIO_PLAYBACK_STATE playbackState =
		static_cast<FMOD_STUDIO_PLAYBACK_STATE>(GetEventPlaybackState(eventName));

	bool isPlaying =
		(playbackState == FMOD_STUDIO_PLAYBACK_PLAYING || playbackState == FMOD_STUDIO_PLAYBACK_SUSTAINING ||
		 playbackState == FMOD_STUDIO_PLAYBACK_STARTING);
	return isPlaying;
}

void AudioSystem::StopAudioEvent(const std::string& eventName, const bool AllowFadeOut)
{
	auto eventInstanceIterator = m_EventInstances.find(eventName);
	if (eventInstanceIterator != m_EventInstances.end())
	{
		FMOD::Studio::EventInstance* eventInstance = eventInstanceIterator->second;

		// Stop and release the EventInstance
		if (!AllowFadeOut)
		{
			[[maybe_unused]] FMOD_RESULT result = eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
			[[maybe_unused]] const char* errorMessage = FMOD_ErrorString(result);
			ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to stop Event: %s", errorMessage);

			result = eventInstance->release();
			ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to release Event Instance: %s", errorMessage);
		}
		else
		{
			[[maybe_unused]] FMOD_RESULT result = eventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
			[[maybe_unused]] const char* errorMessage = FMOD_ErrorString(result);
			ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to stop Event: %s", errorMessage);

			result = eventInstance->release();
			ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to release Event Instance: %s", errorMessage);
		}

		m_EventInstances.erase(eventInstanceIterator); // Remove the event instance from the map
	}
	else
	{
		// Handle error: Event path not found in the map
		ERROR(LOG_AUDIO, "Event not found in map");
	}
}

void Ball::AudioSystem::StopAllAudioEvents()
{
	for (auto it = m_EventInstances.cbegin(); it != m_EventInstances.cend();)
	{
		if (GetIsEventPlaying(it->first))
		{
			StopAudioEvent(it++->first, false);
		}
		else
			++it;
	}
}

void Ball::AudioSystem::SetAllEffects()
{
	SetVolume(m_MasterVolume);
	SetPitch(m_Pitch);
	SetFrequency(m_Frequency);
	SetPan(m_Pan);
	SetFilters(m_Lowpass, m_Highpass);
	SetReverb(m_ReverbLevel, m_ReverbDelay);
	SetChorus(m_ChorusLevel, m_ChorusRate, m_ChorusDepth);
	SetFlanger(m_FlangerDryMix, m_FlangerDepth);
	SetEcho(m_EchoLevel, m_EchoDelay);
}

void Ball::AudioSystem::ResetParameters()
{
	m_MasterVolume = 0.8f;
	m_MusicVolume = 0.5f;
	m_SFXVolume = 0.8f;

	m_Pitch = 0;
	m_Frequency = 0;
	m_Pan = 0;

	m_Lowpass = 0;
	m_Highpass = 0;

	m_ReverbLevel = 0;
	m_ReverbDelay = 0;

	m_ChorusLevel = 0;
	m_ChorusRate = 0;
	m_ChorusDepth = 0;

	m_FlangerDryMix = 0;
	m_FlangerDepth = 0;

	m_EchoLevel = 0;
	m_EchoDelay = 0;
}

void Ball::AudioSystem::SetVolume(float volume)
{
	// Note this function should probably not be used as it forces volume for all audio events < Joey

	m_MasterVolume = volume;

	if (m_Channel)
	{
		// Change volume
		m_Channel->setVolume(volume);
	}

	for (auto& instance : m_EventInstances)
	{
		instance.second->setVolume(volume);
	}
}

void Ball::AudioSystem::SetVolume(const std::string& eventName, float volume)
{
	float MixedVolume = GetMixedVolume(eventName, volume);
	auto eventInstanceIterator = m_EventInstances.find(eventName);
	if (eventInstanceIterator != m_EventInstances.end())
	{
		eventInstanceIterator->second->setVolume(MixedVolume);
	}
}

void Ball::AudioSystem::SetDirectoryVolume(const std::string& EventDirectory, float volume)
{
	bool isMaster = false;
	switch (StringToVolumeType(EventDirectory))
	{
	case VolumeType::Master:
		m_MasterVolume = volume;
		isMaster = true;
		break;
	case VolumeType::Music:
		m_MusicVolume = volume;
		break;
	case VolumeType::SFX:
		m_SFXVolume = volume;
		break;
	default:
		break;
	}

	// Update all volumes since you changed the master
	if (isMaster)
	{
		for (auto& instance : m_EventInstances)
		{
			instance.second->setVolume(GetMixedVolume(instance.first));
		}
	}
	else // Only update the volume specific volume type
	{
		float mixedVolume = GetMixedVolume(EventDirectory);
		for (auto& instance : m_EventInstances)
		{
			if (instance.first.find(EventDirectory) != std::string::npos)
			{
				instance.second->setVolume(mixedVolume);
			}
		}
	}
}

float Ball::AudioSystem::GetDirectoryVolume(const std::string& EventDirectory)
{
	switch (StringToVolumeType(EventDirectory))
	{
	case VolumeType::Master:
		return m_MasterVolume;
	case VolumeType::Music:
		return m_MusicVolume;
	case VolumeType::SFX:
		return m_SFXVolume;
	default:
		break;
	}

	return -1.0f;
}

void Ball::AudioSystem::SetPitch(float pitch)
{
	if (m_Channel)
	{
		// Change pitch
		m_Channel->setPitch(pitch);
	}
}

void Ball::AudioSystem::SetFrequency(float frequency)
{
	if (m_Channel)
	{
		// Change frequency
		FMOD::Sound* sound = nullptr;
		m_Channel->getCurrentSound(&sound);
		if (sound)
		{
			int originalFrequency;
			sound->getDefaults(nullptr, &originalFrequency);
			m_Channel->setFrequency((float)originalFrequency * frequency);
		}
	}
}

void Ball::AudioSystem::SetPan(float pan)
{
	if (m_Channel)
	{
		// Change pan
		m_Channel->setPan(pan);
	}
}

void Ball::AudioSystem::SetFilters(float lowpass, float highpass)
{
	if (m_Channel)
	{
		// Set Lowpass Filter
		FMOD_RESULT result = m_CoreSystem->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &m_DSP);
		const char* errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "failed to create DSB lowpass: %s", errorMessage);
		result = m_Channel->addDSP(0, m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to add DSP to m_Channel: %s", errorMessage);

		result = m_DSP->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, lowpass * m_HertzMultiplier);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "failed to set lowpass cutoff: %s", errorMessage);

		// Set Highpass
		result = m_CoreSystem->createDSPByType(FMOD_DSP_TYPE_HIGHPASS, &m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "Failed to create DSB highpass: %s", errorMessage);
		result = m_Channel->addDSP(0, m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to add DSP to m_Channel: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_HIGHPASS_CUTOFF, highpass * m_HertzMultiplier);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "failed to set highpass cutoff: %s", errorMessage);
	}
}

void Ball::AudioSystem::SetReverb(float reverbLevel, float reverbDelay)
{
	if (m_Channel)
	{
		// Set Reverb
		FMOD_RESULT result = m_CoreSystem->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &m_DSP);
		const char* errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, result == FMOD_OK, "failed to set SFX reverb: %s", errorMessage);
		result = m_Channel->addDSP(0, m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to add DSP to m_Channel: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, 1.0f - reverbLevel);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter drylevel: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY, reverbDelay);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter reverb late delay: %s", errorMessage);
	}
}

void Ball::AudioSystem::SetChorus(float chorusLevel, float chorusRate, float chorusDepth)
{
	if (m_Channel)
	{
		// Set Chorus
		FMOD_RESULT result = m_CoreSystem->createDSPByType(FMOD_DSP_TYPE_CHORUS, &m_DSP);
		const char* errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to create DSP type chorus: %s", errorMessage);
		result = m_Channel->addDSP(0, m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to add DSP to m_Channel: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_CHORUS_MIX, chorusLevel);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter chorus mix: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_CHORUS_RATE, chorusRate);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter chorus rate: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_CHORUS_DEPTH, chorusDepth);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter chorus depth: %s", errorMessage);
	}
}

void Ball::AudioSystem::SetFlanger(float flangerMix, float flangerDepth)
{
	if (m_Channel)
	{
		// Set Flange
		FMOD_RESULT result = m_CoreSystem->createDSPByType(FMOD_DSP_TYPE_FLANGE, &m_DSP);
		const char* errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to create DSP type flanger: %s", errorMessage);
		result = m_Channel->addDSP(0, m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to add DSP to m_Channel: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_FLANGE_MIX, 1.f - flangerMix);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter flanger mix: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_FLANGE_DEPTH, flangerDepth);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter flanger depth: %s", errorMessage);
	}
}

void Ball::AudioSystem::SetEcho(float echoLevel, float echoDelay)
{
	if (m_Channel)
	{
		// Set Echo
		FMOD_RESULT result = m_CoreSystem->createDSPByType(FMOD_DSP_TYPE_ECHO, &m_DSP);
		const char* errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to create DSP type echo: %s", errorMessage);
		result = m_Channel->addDSP(0, m_DSP);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to add DSP to m_Channel: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_ECHO_DELAY, echoDelay);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter echo delay: %s", errorMessage);
		result = m_DSP->setParameterFloat(FMOD_DSP_ECHO_DRYLEVEL, 1.f - echoLevel);
		errorMessage = FMOD_ErrorString(result);
		ASSERT_MSG(LOG_AUDIO, FMOD_OK, "Failed to set DSP parameter echo drylevel: %s", errorMessage);
	}
}

float Ball::AudioSystem::GetMixedVolume(const std::string eventName, float volume)
{
	float MixedVolume = volume * m_MasterVolume;

	switch (StringToVolumeType(eventName))
	{
	case VolumeType::Music:
		return MixedVolume * m_MusicVolume;
		break;
	case VolumeType::SFX:
		return MixedVolume * m_SFXVolume;
		break;
	default:
		break;
	}

	return volume * m_MasterVolume;
}
