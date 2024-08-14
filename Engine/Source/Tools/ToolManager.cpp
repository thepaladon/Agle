#include "Tools/ToolManager.h"
#include "ImGui/imgui.h"

#include "Engine.h"
#include "Window.h"
#include "Input/Input.h"

#include "Tools/BindlessHeapViewer.h"
#include "Tools/CameraSettings.h"
#include "Tools/StepTool.h"

#include "Tools/InputViewTool.h"
#include "Tools/RenderModeUI.h"
#include "Tools/TonemapperSettings.h"
#include "Tools/BloomSettingsUI.h"
#include "Tools/GpuMarkerVisualizer.h"
#include "Tools/SceneCompare.h"
#include "Tools/GridSettingsUI.h"
#include "Tools/AudioParameters.h"
#include "Utilities/LaunchParameters.h"

#include <sstream>

#include "Tools/BufferVisualizer.h"

using namespace Ball;

#define REGISTER_TOOL(TYPE) RegisterTool<TYPE>(#TYPE)

void ToolManager::Init()
{
#ifdef NO_IMGUI
	return;
#endif

	m_Tools.clear();
	REGISTER_TOOL(BindlessHeapViewer);
	REGISTER_TOOL(CameraSettings);
	REGISTER_TOOL(StepTool);
	REGISTER_TOOL(SceneCompare);
	REGISTER_TOOL(RenderModeUI);
	REGISTER_TOOL(GridSettingsUI);
	REGISTER_TOOL(BloomSettingsUI);
	REGISTER_TOOL(TonemapperSettings);
	REGISTER_TOOL(GpuMarkerVisualizer);
	REGISTER_TOOL(AudioParameter);
	REGISTER_TOOL(InputViewTool);
	REGISTER_TOOL(BufferVisualizer);

	std::string source = Ball::LaunchParameters::GetString("OpenTools", "");
	if (!source.empty())
	{
		std::stringstream ss(source);
		std::string toolName;
		while (!ss.eof())
		{
			getline(ss, toolName, ',');
			OpenTool(toolName.c_str());
		}
	}
}

void Ball::ToolManager::Shutdown()
{
#ifndef NO_IMGUI
	for (size_t i = 0; i < m_Tools.size(); i++)
	{
		delete m_Tools[i];
	}
#endif // !NO_IMGUI
}

void ToolManager::CreateToolMenu(const char* menuName, ToolCatagory category) const
{
	if (ImGui::BeginMenu(menuName))
	{
		for (size_t i = 0; i < m_Tools.size(); i++)
		{
			ToolBase* tool = m_Tools[i];
			if (tool->GetToolCatagory() == category)
			{
				if (ImGui::MenuItem(tool->GetName().c_str()))
				{
					if (tool->GetInterfaceType() == ToolInterfaceType::WINDOW)
						tool->ToggleOpen();
					else
						tool->Event();
				}
			}
		}
		ImGui::EndMenu();
	}
}

// Use an array of menu names and categories to loop through and create each menu
std::vector<std::pair<const char*, ToolCatagory>> m_MenuItems = {
	{"Engine", ToolCatagory::ENGINE},
	{"Graphics", ToolCatagory::GRAPHICS},
	{"Resources", ToolCatagory::RESOURCES},
	{"Audio", ToolCatagory::AUDIO},
};

void ToolManager::OnImgui()
{
	if (GetEngine().GetInput().GetActionDown("ToolOverlay"))
	{
		m_ShowToolManager = !m_ShowToolManager;
	}

	// Dont draw Toolbar and info bottom bar when toolmanager is hidden
	if (!m_ShowToolManager)
		return;

	// Drawing the Windows and updatign the tools
	{
		if (m_ShowDemoWindow)
			ImGui::ShowDemoWindow();

		// Loop over all the tools and if their show flag is set draw them
		for (size_t i = 0; i < m_Tools.size(); i++)
		{
			ToolBase* tool = m_Tools[i];
			tool->Update();
			if (tool->IsOpen())
				tool->Draw();
		}
	}

	// Draw the tool bar
	{
		ImGui::BeginMainMenuBar();

		for (const auto& menu : m_MenuItems)
		{
			CreateToolMenu(menu.first, menu.second);
		}

		ImGui::Separator();
		ImGui::Text("Hide/Show (F1) | Toggle Freecam (F2)");

		float SidePadding = 150.0f;
		ImGui::SetCursorPosX(GetEngine().GetWindow().GetWidth() - SidePadding);
		ImGui::Separator();
		ImGui::Text("Delta time: %.3f", GetEngine().GetDeltaTime());

		ImGui::EndMainMenuBar();
	}
}
void ToolManager::OpenTool(const std::string& name) const
{
	if (m_ToolLookup.find(name) == m_ToolLookup.end())
	{
		ERROR(LOG_LAUNCHPARAM, "Tool named '%s' does not exist, make sure to use the tools class name!", name.c_str());
		return;
	}

	m_ToolLookup.find(name)->second->Open();
}
