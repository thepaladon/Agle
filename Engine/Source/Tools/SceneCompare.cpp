#include "Tools/SceneCompare.h"
#include "Tools/ToolManager.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "Engine.h"
#include "Log.h"
#include "Levels/Level.h"
#include "FileIO.h"

#include <External/nlohmann/json.hpp>

#include "Utilities/LaunchParameters.h"
#include "Rendering/Renderer.h"
#include "GameObjects/Types/Camera.h"
#include "Rendering/BEAR/Texture.h"

using namespace Ball;

void SceneCompare::Init()
{
	m_Name = "Scene Compare";
	m_ToolCatagory = ToolCatagory::GRAPHICS;
	m_ToolInterfaceType = ToolInterfaceType::WINDOW;

	if (LaunchParameters::Contains("SceneCompare"))
	{
		m_LoadOnStart = true;
		m_Open = true;
		m_StateFileName = LaunchParameters::GetString("SceneCompare", "DefaultState.json");
	}
}

void SceneCompare::Draw()
{
	RenderAPI& renderer = GetEngine().GetRenderer();
	Camera* camera = Ball::Camera::GetActiveCamera();

	const glm::vec3 position = camera->GetTransform().GetPosition();
	const glm::vec3 rotation = camera->GetTransform().GetEulerAngles();

	ImGui::Begin(m_Name.c_str(), &m_Open);

	// Input field for which SaveState to load
	ImGui::InputText("File name", &m_StateFileName);

	if (ImGui::Button("Save state"))
	{
		nlohmann::json data;

		data["CameraPosition"] = {position.x, position.y, position.z};
		data["CameraRotation"] = {rotation.x, rotation.y, rotation.z};
		data["RenderingMode"] = static_cast<int>(renderer.m_RenderMode);

		FileIO::Write(FileIO::DirectoryType::TempData, m_StateFileName, data.dump());

		LOG(LOG_LOGGING, "SceneCompare: Saved state [%s]", m_StateFileName.c_str());
	}

	ImGui::SameLine();

	if (ImGui::Button("Load state") || m_LoadOnStart)
	{
		m_LoadOnStart = false;

		if (FileIO::Exist(FileIO::DirectoryType::TempData, m_StateFileName))
		{
			std::string readData = FileIO::Read(FileIO::DirectoryType::TempData, m_StateFileName);
			nlohmann::json data = nlohmann::json::parse(readData);

			// Check if parsed data is empty
			if (!data.empty())
			{
				// Load camera position
				if (data.contains("CameraPosition"))
				{
					glm::vec3 newPosition = {0, 0, 0};
					newPosition.x = data.at("CameraPosition").at(0);
					newPosition.y = data.at("CameraPosition").at(1);
					newPosition.z = data.at("CameraPosition").at(2);
					camera->GetTransform().SetPosition(newPosition);
				}

				// Load camera rotation
				if (data.contains("CameraRotation"))
				{
					glm::vec3 newRotation = {0, 0, 0};
					newRotation.x = data.at("CameraRotation").at(0);
					newRotation.y = data.at("CameraRotation").at(1);
					newRotation.z = data.at("CameraRotation").at(2);
					camera->GetTransform().SetRotation(newRotation);
				}

				// Load rendering mode
				if (data.contains("RenderingMode"))
				{
					renderer.m_RenderMode = static_cast<RenderModes>(data.at("RenderingMode"));
				}

				LOG(LOG_LOGGING, "SceneCompare: Loaded state [%s]", m_StateFileName.c_str());
			}
		}
	}

	// Display camera position
	ImGui::Text("Camera position: x=%f, y=%f, z=%f", position.x, position.y, position.z);

	// Display camera rotation
	ImGui::Text("Camera rotation: x=%f, y=%f, z=%f", rotation.x, rotation.y, rotation.z);

	// Display rendering mode
	std::string renderModeText = "null";
	switch (renderer.m_RenderMode)
	{
	case RenderModes::RM_RAY_TRACE:
		renderModeText = "Ray tracing";
		break;
	case RenderModes::RM_ALBEDO:
		renderModeText = "Albedo";
		break;
	case RenderModes::RM_NORMAL:
		renderModeText = "Normal";
		break;
	case RenderModes::RM_UV:
		renderModeText = "UV";
		break;
	case RenderModes::RM_PATH_TRACE:
		renderModeText = "Path tracing";
		break;
	case RenderModes::NOT_IMLPEMENTED:
		renderModeText = "Not implemented";
		break;
	default:
		break;
	}

	ImGui::Text("Rendering mode: %s", renderModeText.c_str());

	ImGui::End();
}

void SceneCompare::Update()
{
}

void SceneCompare::Event()
{
}