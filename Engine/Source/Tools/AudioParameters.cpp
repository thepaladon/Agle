#include "Tools/AudioParameters.h"

#include "AudioSystem.h"
#include <ImGui/imgui.h>
#include "Engine.h"

void Ball::AudioParameter::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	// Update scene parameters
	ImGui::Text("Audio Parameters:");
	const char* playPauseButtonText = m_Playing ? "Stop" : "Start";
	if (ImGui::Button(playPauseButtonText))
	{
		m_Playing = !m_Playing;
		if (m_Playing)
		{
			GetEngine().GetAudio().PlayAudioEvent("Music/TestEvent");
		}
		else
		{
			GetEngine().GetAudio().StopAudioEvent("Music/TestEvent", false);
		}
	}
	const char* effectSwitch = GetEngine().GetAudio().m_Applyeffects ? "Apply effects" : "stop effects";
	if (ImGui::Button(effectSwitch))
	{
		GetEngine().GetAudio().m_Applyeffects = GetEngine().GetAudio().m_Applyeffects;
	}

	ImGui::Separator();

	// Volume control
	float masterVolume = GetEngine().GetAudio().GetDirectoryVolume("Master");
	if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f))
		GetEngine().GetAudio().SetDirectoryVolume("Master", masterVolume);

	float musicVolume = GetEngine().GetAudio().GetDirectoryVolume("Music");
	if (ImGui::SliderFloat("Music Volume", &musicVolume, 0.0f, 1.0f))
		GetEngine().GetAudio().SetDirectoryVolume("Music", musicVolume);

	float sfxVolume = GetEngine().GetAudio().GetDirectoryVolume("SFX");
	if (ImGui::SliderFloat("SFX Volume", &sfxVolume, 0.0f, 1.0f))
		GetEngine().GetAudio().SetDirectoryVolume("SFX", sfxVolume);

	ImGui::Separator();

	// ImGui::SliderFloat("Pitch", &GetEngine().GetAudio().m_Pitch, 0.f, 1.0f);
	//  ImGui::SliderFloat("Frequency", &GetEngine().GetAudio().m_Frequency, 0.f, 1.0f);
	ImGui::DragFloat("Pan", &GetEngine().GetAudio().m_Pan, 1, -1.f, 1.f);
	ImGui::Separator();
	ImGui::DragFloat("High Pass", &GetEngine().GetAudio().m_Highpass, 1, 1.f, 22000);
	ImGui::DragFloat("Low Pass", &GetEngine().GetAudio().m_Lowpass, 1, 1.f, 22000.f);
	ImGui::Separator();
	ImGui::DragFloat("Reverb Level", &GetEngine().GetAudio().m_ReverbLevel, 1.f, -80.f, 20.f);
	ImGui::DragFloat("Reverb Delay", &GetEngine().GetAudio().m_ReverbDelay, 1.f, 0.f, 100.f);
	ImGui::Separator();
	ImGui::DragFloat("Chorus Level", &GetEngine().GetAudio().m_ChorusLevel, 1.f, 0.f, 100.f);
	ImGui::DragFloat("Chorus Depth", &GetEngine().GetAudio().m_ChorusDepth, 1.f, 0.f, 100.f);
	ImGui::DragFloat("Chorus Rate", &GetEngine().GetAudio().m_ChorusRate, 1.f, 0.f, 20.f);
	ImGui::Separator();
	ImGui::DragFloat("Flanger Dry Mix", &GetEngine().GetAudio().m_FlangerDryMix, 1.f, 0.f, 50.f);
	ImGui::DragFloat("Flanger Depth", &GetEngine().GetAudio().m_FlangerDepth, .001f, 0.01f, 1.f);
	ImGui::Separator();
	ImGui::DragFloat("Echo Level", &GetEngine().GetAudio().m_EchoLevel, 1.f, -80.f, 10.f);
	ImGui::DragFloat("Echo Delay", &GetEngine().GetAudio().m_EchoDelay, 10.f, 10.f, 5000.f);
	if (ImGui::Button("Reset parameters"))
	{
		GetEngine().GetAudio().ResetParameters();
	}
	ImGui::End();
}

void Ball::AudioParameter::Update()
{
}
