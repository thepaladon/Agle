#include "ImGui/imgui.h"
#include "Tools/StepTool.h"
#include "Engine.h"

using namespace Ball;

void StepTool::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	ImGui::Text("Current frame: %i", m_FrameCount + m_AdvancedFrameCount);
	ImGui::Text("Advanced frame count: %i ", m_AdvancedFrameCount);
	ImGui::Text("Delta time between last and this frame: %f", m_PreviousDeltaTime);

	// Make button toggle the pause state and display the name of the button based on pause state
	const char* playPauseButtonText = m_Paused ? "Continue" : "Pause";
	if (ImGui::Button(playPauseButtonText))
	{
		m_Paused = !m_Paused;
		GetEngine().PauseGame(m_Paused);

		// Reset advanced frame count when unpaused
		if (!m_Paused)
		{
			m_AdvancedFrameCount = 0;
			m_PreviousDeltaTime = 0;
		}
	}

	ImGui::SameLine();

	// Only allow next frame button to be pressed when is paused
	if (m_Paused)
	{
		if (ImGui::Button("Next frame") && m_Paused)
		{
			m_AdvancedFrameCount++;
			m_PreviousDeltaTime = GetEngine().GetDeltaTime();
		}
	}

	ImGui::End();
}

void StepTool::Update()
{
	if (!m_Paused)
	{
		m_FrameCount++;
	}
}
