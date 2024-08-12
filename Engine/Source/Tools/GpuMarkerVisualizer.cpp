#include "Tools/GpuMarkerVisualizer.h"
#include "Engine.h"
#include "Rendering/Renderer.h"

#include <ImGui/imgui.h>

using namespace Ball;

ImVec4 GetColorFromValue(float value, float max_value)
{
	// Normalize the input value to a 0-1 scale
	float normTime = std::clamp(value / max_value, 0.0f, 1.0f);
	ImVec4 color;

	// Determine the color based on the normalized time
	if (normTime <= 0.5)
	{
		// Interpolate between green (0,1,0) and yellow (1,1,0)
		float factor = normTime / 0.5; // Scale to 0-1 range
		color = ImVec4(factor, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		// Interpolate between yellow (1,1,0) and red (1,0,0)
		float factor = (normTime - 0.5) / 0.5; // Scale to 0-1 range
		color = ImVec4(1.0f, 1.0f - factor, 0.0f, 1.0f);
	}

	return color;
}

void GpuMarkerVisualizer::Init()
{
	m_Open = false;
	m_Name = "GPU Profiler";
	m_ToolCatagory = ToolCatagory::GRAPHICS;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
}

void GpuMarkerVisualizer::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	RenderAPI& renderer = GetEngine().GetRenderer();

	ImGui::DragFloat("Color Sensitivity Clamp (ms)", &m_ColorSensitivityClamp, 0.05f, 0.0f, 32.f);

	// Update this each frame
	// Iterate over each unique entry in the history map
	if (ImGui::CollapsingHeader("Histograms"))
	{
		static bool paused = false;
		ImGui::Checkbox("Pause Update", &paused);

		for (auto& t : renderer.m_Data)
		{
			if (!paused)
			{
				historyMap[t.name].push_back(t.timeInMs);

				// Optional: Limit the history size to keep the last N samples
				if (historyMap[t.name].size() > 100)
				{ // keep last 100 entries
					historyMap[t.name].erase(historyMap[t.name].begin());
				}
			}

			const std::string& name = t.name;
			std::vector<float>& values = historyMap[t.name];

			if (!values.empty())
			{
				// Calculate the color based on the most recent value
				float lastValue = values.back();
				ImVec4 color = GetColorFromValue(lastValue, m_ColorSensitivityClamp);
				ImGui::PushStyleColor(ImGuiCol_PlotLines, color);

				// Plot the line graph for this entry
				ImGui::PlotLines(
					name.c_str(), values.data(), values.size(), 0, NULL, 0, m_ColorSensitivityClamp, ImVec2(0, 120));

				// Pop the color style
				ImGui::PopStyleColor();
			}
		}
	}

	for (auto& t : renderer.m_Data)
	{
		// ImGui::Text("%s: %f", t.name.c_str(), t.timeInMs);

		ImVec4 color = GetColorFromValue(t.timeInMs, m_ColorSensitivityClamp);

		// Push the color for the text
		ImGui::PushStyleColor(ImGuiCol_Text, color);

		// Display the text
		ImGui::Text("%s: %f ms", t.name.c_str(), t.timeInMs);

		// Pop the color style to reset it
		ImGui::PopStyleColor();
	}

	ImGui::End();
}
