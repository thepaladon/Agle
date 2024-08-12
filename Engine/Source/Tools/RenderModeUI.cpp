#include "Tools/RenderModeUI.h"
#include "Engine.h"
#include "Rendering/Renderer.h"
#include "Rendering/Denoiser.h"
#include "Rendering/ModelLoading/ModelManager.h"
#include "Utilities/LaunchParameters.h"
#include "Utilities/RenderUtilities.h"

#include <ImGui/imgui.h>

using namespace Ball;

void RenderModeUI::Init()
{
	m_Name = "Render Modes";
	m_ToolCatagory = ToolCatagory::GRAPHICS;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
	m_Open = false;
	const size_t arraySize = std::size(m_RenderModes);
	const auto launchParamRenderMode =
		LaunchParameters::GetString("RenderMode", ""); // No Fallback as for-loop accounts for it

	// Check whether a render mode like this exists
	// If nothing is found resort to using default value of m_RenderMode set where it lives
	for (size_t i = 0; i < arraySize; ++i)
	{
		if (strcmp(m_RenderModes[i], launchParamRenderMode.c_str()) == 0)
		{
			GetEngine().GetRenderer().m_RenderMode = static_cast<RenderModes>(i);
			LOG(LOG_GRAPHICS, "Launch Parameter RenderMode: %s", launchParamRenderMode.c_str());
			break;
		}
	}
}

void RenderModeUI::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	RenderAPI& renderer = GetEngine().GetRenderer();

	if (ImGui::Button("Take Screenshot"))
	{
		renderer.TakeScreenshot("TestName.png");
	}

	ImGui::Checkbox("Wireframe Debug", &renderer.m_DrawWireframe);
	ImGui::Text("Total Frames: %i", renderer.m_NumTotalFrames);
	ImGui::DragFloat("m_BrightnessThreshold", &renderer.m_BrightnessThreshold, 0.1f, 0.1f, 100.f);
	ImGui::DragFloat("HDRI Lighting Strength", &renderer.m_HDRILightingStrength, 0.1f, 0.f, 10.f);
	ImGui::DragFloat("HDRI Background Strength", &renderer.m_HDRIBackgroundStrength, 0.1f, 0.f, 10.f);
	ImGui::DragInt("Ray Bounces:", (int*)&renderer.m_MaxRecursionDepth, 1.f, 1, 20);

	// Combo box for render mode selection
	if (ImGui::Combo("Render Mode", &m_SelectedRenderMode, m_RenderModes, (int)RenderModes::RenderModeCount))
	{
		renderer.m_RenderMode = static_cast<RenderModes>(m_SelectedRenderMode);
		renderer.m_ShouldClearAccum = true;
	}

	ImGui::Checkbox("Reprojection", &renderer.m_ReprojectionEnabled);
	if (renderer.m_ReprojectionEnabled)
	{
		ImGui::DragFloat("Reprojection Alpha", &renderer.GetDenoiserPtr()->m_Alpha, 0.001f, 0.0f, 1.f);
		ImGui::DragFloat(
			"Reprojection Normal Deviation", &renderer.GetDenoiserPtr()->m_NormalThreshold, 0.001f, 0.0f, 1.f);
	}

	ImGui::DragFloat("Tracing Distance", &renderer.m_TracingDistanceMultiplier, 1.f, 0.01f, 400.f);

	ImGui::Checkbox("Denoising", &renderer.m_DenoisingEnabled);

	if (ImGui::CollapsingHeader("Accumulation:"))
	{
		ImGui::Checkbox("Stationary Accumulation", &renderer.m_StationaryCamAccumEnabled);
		ImGui::Checkbox("Frame Cap", &renderer.m_AccumFrameCapEnabled);
		ImGui::Text("Accumulated Frames: %i", renderer.m_AccumFramesNum);

		if (renderer.m_AccumFrameCapEnabled)
			ImGui::DragInt("Accumulated Frames Target", &renderer.m_MaxAccumulatedFrames, 0.25f, 1, 4096, "%i Frames");
	}

	if (ImGui::CollapsingHeader("ReSTIR:"))
	{
		ImGui::Text("Evaluated Lights: %i", renderer.GetModelManager()->GetNumLightsInScene());
		auto& restirSettings = renderer.m_ReStirSettings;
		int currentItem = restirSettings.m_UseReSTIR; // Ensure this variable reflects the current state of your setting
		const char* items = "Off \0 RIS Only \0 RIS + Temporal \0  RIS + Spatial \0  RIS + Temporal + Spatial\0";
		if (ImGui::Combo("ReSTIR Settings", &currentItem, items))
		{
			// Update your setting based on the new selection
			restirSettings.m_UseReSTIR = currentItem;
			renderer.m_ShouldClearAccum = true;
		}
		ImGui::Text("Selected setting: %d", restirSettings.m_UseReSTIR);
		ImGui::DragInt("Num Lights for RIS", &restirSettings.m_RISRandomLights, 0.25f, 0, 1024);
		ImGui::DragInt("Previous Frame Contribution Cap", &restirSettings.m_CurrentLightClamp, 0.5f, 0);

		ImGui::DragFloat("Normal Threshold", &renderer.m_NormalThresholdReSTIR, 0.01f, 0.f, 1.f);
		ImGui::DragFloat("Depth Threshold", &renderer.m_DepthThreshold, 0.01f, 0.f, 1.f);
		ImGui::DragInt("Num Spatial Samples", &renderer.m_NumSpatialSamples, 0.0, 10);
		ImGui::DragFloat("Spatial Radius", &renderer.m_SpatialRadius, 0.5f, 0.5f, 100.0f);
	}

	auto d = renderer.m_Denoiser;

	ImGui::DragInt("Filter Iterations", &d->m_FilterIterations, 1, 1, 7);

	if (ImGui::Checkbox("Demodulate Indirect", &d->m_DebugSettings.m_DemodulateIndirectLighting))
		d->m_DebugSettings.m_DemodulateDirectLighting = false;
	if (ImGui::Checkbox("Demodulate Direct", &d->m_DebugSettings.m_DemodulateDirectLighting))
		d->m_DebugSettings.m_DemodulateIndirectLighting = false;

	ImGui::DragFloat("Moments Alpha", &d->m_MomentsAlpha, 0.001f, 0.f, 1.f);
	ImGui::Checkbox("Visualize Variance", &d->m_DebugSettings.m_VisualizeVariance);
	ImGui::Checkbox("Visualize Weights", &d->m_DebugSettings.m_VisualizeWeights);
	ImGui::DragInt("History", &d->m_History, 1, 4, 128);
	ImGui::DragFloat("Phi illum", &d->m_PhiIllumination, 0.1f, 0.01f, 100.f);
	ImGui::DragFloat("Phi norm ", &d->m_PhiNormal, 0.001f, 0.f, 1.f);

	ImGui::Checkbox("Grid Shader", &renderer.m_RunGridShader);
	ImGui::DragFloat("Line Thickness", &renderer.m_LineThickness, 0.1f, 1.f, 3.f);
	ImGui::ColorEdit3("Selected Line Color", &renderer.m_OutlinesSelectedColor[0]);
	ImGui::ColorEdit3("Hovered Line Color", &renderer.m_OutlinesHoveredColor[0]);

	// Animations
	renderer.GetModelManager()->AnimationImGui();
	ImGui::End();
}
