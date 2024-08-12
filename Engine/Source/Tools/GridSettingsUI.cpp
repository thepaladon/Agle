#include "Tools/GridSettingsUI.h"
#include "Engine.h"
#include "Rendering/Renderer.h"

#include <ImGui/imgui.h>

using namespace Ball;

void GridSettingsUI::Init()
{
	m_Open = false;
	m_Name = "Grid Settings";
	m_ToolCatagory = ToolCatagory::GRAPHICS;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;
}

void GridSettingsUI::Draw()
{
	ImGui::Begin(m_Name.c_str(), &m_Open);

	RenderAPI& renderer = GetEngine().GetRenderer();

	auto& v = renderer.m_GridSettings;

	ImGui::Checkbox("Grid Shader", &renderer.m_RunGridShader);
	ImGui::DragFloat3("Grid Position", &v.m_GridOffset.x, 0.25f);
	ImGui::DragFloat2("Grid Size", &v.m_GridSize.x, 0.1f, 0.f, 100000.f);
	ImGui::DragFloat("Line Width", &v.m_LineWidth, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("Opacity", &v.m_Opacity, 0.01f, 0.0f, 50.f);

	ImGui::End();
}
