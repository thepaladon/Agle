#include <Catch2/catch_amalgamated.hpp>
#include <../External/FMOD/inc/fmod_studio_common.h>
#include "AudioSystem.h"

#include <../External/FMOD/inc/fmod_errors.h>
#include <../External/FMOD/inc/fmod.hpp>
#include <../External/FMOD/inc/fmod_studio.hpp>
#include <../External/FMOD/inc/fmod_common.h>

using namespace Ball;

CATCH_TEST_CASE("Audio unit tests")
{
	AudioSystem m_Audio;
	m_Audio.Init();

	FMOD::Studio::Bank* bank1 = nullptr;
	FMOD::Studio::Bank* bank2 = nullptr;
	FMOD::Studio::Bank* bank3 = nullptr;

	CATCH_SECTION("Initialization of FMOD")
	{
		CATCH_REQUIRE(m_Audio.GetStudioSystem() != nullptr);
	}

	CATCH_SECTION("Load Banks")
	{
		bank1 = m_Audio.LoadBank("Master.bank");
		bank2 = m_Audio.LoadBank("Master.strings.bank");
		bank3 = m_Audio.LoadBank("Music.bank");

		CATCH_REQUIRE(bank1 != nullptr);
		CATCH_REQUIRE(bank2 != nullptr);
		CATCH_REQUIRE(bank3 != nullptr);
	}

	CATCH_SECTION("Audio Events")
	{
		CATCH_SECTION("Create Event")
		{
			bank1 = m_Audio.LoadBank("Master.bank");
			bank2 = m_Audio.LoadBank("Master.strings.bank");
			bank3 = m_Audio.LoadBank("SFX.bank");
			CATCH_REQUIRE(m_Audio.CreateAudioEvent("SFX/PlayerHit") != nullptr);
		}
		CATCH_SECTION("Play Event")
		{
			bank1 = m_Audio.LoadBank("Master.bank");
			bank2 = m_Audio.LoadBank("Master.strings.bank");
			bank3 = m_Audio.LoadBank("SFX.bank");
			m_Audio.PlayAudioEvent("SFX/PlayerHit");

			CATCH_REQUIRE(m_Audio.GetEventPlaybackState("SFX/PlayerHit") == FMOD_STUDIO_PLAYBACK_STARTING);
		}
		CATCH_SECTION("Stop Event")
		{
			bank1 = m_Audio.LoadBank("Master.bank");
			bank2 = m_Audio.LoadBank("Master.strings.bank");
			bank3 = m_Audio.LoadBank("SFX.bank");

			m_Audio.PlayAudioEvent("SFX/PlayerHit");
			m_Audio.StopAudioEvent("SFX/PlayerHit", true);
			CATCH_REQUIRE(m_Audio.GetEventInstances().size() == 0);
		}
	}
}
