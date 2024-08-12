#include "ImGui/imgui.h"
#include "Tools/CameraSettings.h"
#include "Tools/ToolManager.h"
#include "Engine.h"
#include "Rendering/Renderer.h"
#include "GameObjects/Types/Camera.h"
#include "GameObjects/Types/FreeCamera.h"
#include "GameObjects/Types/LevelEditorCamera.h"

using namespace Ball;

void CameraSettings::Draw()
{
	Camera* camera = Camera::GetActiveCamera();
	ImGui::Begin(m_Name.c_str(), &m_Open);

	glm::vec location = camera->GetTransform().GetPosition();
	if (ImGui::DragFloat3("Location", &location.x))
	{
		camera->GetTransform().SetPosition(location);
	}

	auto freeCam = dynamic_cast<FreeCamera*>(camera);
	if (freeCam != nullptr)
	{
		ImGui::DragFloat("Camera Move Scalar", &freeCam->m_MoveScalar, 0.001f, 0.0f, 1.f);
		ImGui::DragFloat("Camera View Scalar", &freeCam->m_ViewScalar, 0.001f, 0.0f, 1.f);
	}

	auto editorCam = dynamic_cast<LevelEditorCamera*>(camera);
	if (editorCam != nullptr)
	{
		ImGui::DragFloat("Camera Move Scalar", &editorCam->m_MoveScalar, 0.001f, 0.0f, 1.f);
		ImGui::DragFloat("Camera View Scalar", &editorCam->m_ViewScalar, 0.001f, 0.0f, 1.f);
	}

	ImGui::DragFloat("Cam FOV:", &camera->m_FOV, 0.25f, 0.01f, 180.f);

	ImGui::Text("Toggle frame accumulation press '1'");
	ImGui::Text("Toggle path tracing press '2'");
	ImGui::End();
}
