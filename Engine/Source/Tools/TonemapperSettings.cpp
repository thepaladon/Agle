#include "Tools/TonemapperSettings.h"
#include "Engine.h"
#include "Rendering/Renderer.h"

#include <ImGui/imgui.h>

using namespace Ball;

static const int VALUES_COUNT = 256;
static const float HDR_MAX = 12.0f;
static TonemapParameters* g_Tonemapping = nullptr;

float LinearTonemapping(float HDR, float max)
{
	if (max > 0.0f)
	{
		return glm::clamp(HDR / max, 0.0f, 1.0f);
	}
	return HDR;
}

float LinearTonemappingPlot(void*, int index)
{
	return LinearTonemapping(index / (float)VALUES_COUNT * HDR_MAX, g_Tonemapping->m_MaxLuminance);
};

float ReinhardTonemapping(float HDR, float k)
{
	return HDR / (HDR + k);
}

float ReinhardTonemappingPlot(void*, int index)
{
	return ReinhardTonemapping(index / (float)VALUES_COUNT * HDR_MAX, g_Tonemapping->m_ReinhardConstant);
}

float ReinhardSqrTonemappingPlot(void*, int index)
{
	float reinhard = ReinhardTonemapping(index / (float)VALUES_COUNT * HDR_MAX, g_Tonemapping->m_ReinhardConstant);
	return reinhard * reinhard;
}

float ACESFilmicTonemapping(float x, float A, float B, float C, float D, float E, float F)
{
	return (((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F));
}

float ACESFilmicTonemappingPlot(void*, int index)
{
	float HDR = index / (float)VALUES_COUNT * HDR_MAX;
	return ACESFilmicTonemapping(HDR,
								 g_Tonemapping->m_ShoulderStrength,
								 g_Tonemapping->m_LinearStrength,
								 g_Tonemapping->m_LinearAngle,
								 g_Tonemapping->m_ToeStrength,
								 g_Tonemapping->m_ToeNumerator,
								 g_Tonemapping->m_ToeDenominator) /
		ACESFilmicTonemapping(g_Tonemapping->m_LinearWhite,
							  g_Tonemapping->m_ShoulderStrength,
							  g_Tonemapping->m_LinearStrength,
							  g_Tonemapping->m_LinearAngle,
							  g_Tonemapping->m_ToeStrength,
							  g_Tonemapping->m_ToeNumerator,
							  g_Tonemapping->m_ToeDenominator);
}

void TonemapperSettings::Init()
{
	m_Open = false;
	m_Name = "Tonemapper Parameters";
	m_ToolCatagory = ToolCatagory::GRAPHICS;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
}

void TonemapperSettings::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	RenderAPI& renderer = GetEngine().GetRenderer();

	g_Tonemapping = &renderer.m_TonemapParams;

	ImGui::SliderFloat("Exposure", &g_Tonemapping->m_Exposure, -10.0f, 10.0f);
	ImGui::SliderFloat("Gamma", &g_Tonemapping->m_Gamma, 0.01f, 5.0f);

	ImGui::Spacing();
	const char* modes[] = {"Linear", "Reinhard", "Reinhard Squared", "ACES Filmic"};
	ImGui::Combo("Tonemapping Methods", (int*)(&g_Tonemapping->m_TonemapMethod), modes, 4);

	switch (g_Tonemapping->m_TonemapMethod)
	{
	case TM_LINEAR:
		ImGui::PlotLines(
			"Linear Tonemapping", LinearTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
		ImGui::SliderFloat("Max Brightness", &g_Tonemapping->m_MaxLuminance, 1.0f, 10.0f);
		break;
	case TM_REINHARD:
		ImGui::PlotLines("Reinhard Tonemapping",
						 &ReinhardTonemappingPlot,
						 nullptr,
						 VALUES_COUNT,
						 0,
						 nullptr,
						 0.0f,
						 1.0f,
						 ImVec2(0, 250));
		ImGui::SliderFloat("Reinhard Constant", &g_Tonemapping->m_ReinhardConstant, 0.01f, 10.0f);
		break;
	case TM_REINHARDSQ:
		ImGui::PlotLines("Reinhard Squared Tonemapping",
						 &ReinhardSqrTonemappingPlot,
						 nullptr,
						 VALUES_COUNT,
						 0,
						 nullptr,
						 0.0f,
						 1.0f,
						 ImVec2(0, 250));
		ImGui::SliderFloat("Reinhard Constant", &g_Tonemapping->m_ReinhardConstant, 0.01f, 10.0f);
		break;
	case TM_ACESFILMIC:
		ImGui::PlotLines("ACES Filmic Tonemapping",
						 &ACESFilmicTonemappingPlot,
						 nullptr,
						 VALUES_COUNT,
						 0,
						 nullptr,
						 0.0f,
						 1.0f,
						 ImVec2(0, 250));
		ImGui::SliderFloat("Shoulder Strength", &g_Tonemapping->m_ShoulderStrength, 0.01f, 5.0f);
		ImGui::SliderFloat("Linear Strength", &g_Tonemapping->m_LinearStrength, 0.0f, 10.0f);
		ImGui::SliderFloat("Linear Angle", &g_Tonemapping->m_LinearAngle, 0.0f, 1.0f);
		ImGui::SliderFloat("Toe Strength", &g_Tonemapping->m_ToeStrength, 0.01f, 1.0f);
		ImGui::SliderFloat("Toe Numerator", &g_Tonemapping->m_ToeNumerator, 0.0f, 10.0f);
		ImGui::SliderFloat("Toe Denominator", &g_Tonemapping->m_ToeDenominator, 1.0f, 10.0f);
		ImGui::SliderFloat("Linear White", &g_Tonemapping->m_LinearWhite, 1.0f, 120.0f);
		break;
	default:
		break;
	}

	ImGui::End();
}
