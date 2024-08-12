#include "Tools/BloomSettingsUI.h"
#include "Engine.h"
#include "Rendering/Renderer.h"

#include <ImGui/imgui.h>

using namespace Ball;

void BloomSettingsUI::Init()
{
	m_Open = false;
	m_Name = "Bloom Settings";
	m_ToolCatagory = ToolCatagory::GRAPHICS;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
}

void BloomSettingsUI::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	RenderAPI& renderer = GetEngine().GetRenderer();

	auto& v = renderer.m_BloomSettings;

	ImGui::Checkbox("Bloom Enabled", &v.m_Enabled);
	ImGui::DragFloat("Radius", &v.m_Radius, 0.01f, 0.f);
	ImGui::DragFloat("Intensity", &v.m_Intensity, 0.01f, 0.0f);

	ImGui::End();
}
