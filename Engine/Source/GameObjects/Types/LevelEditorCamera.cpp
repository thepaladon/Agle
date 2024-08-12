#include "GameObjects/Types/LevelEditorCamera.h"
#include "Engine.h"
#include "Input/Input.h"
#include "GameObjects/Types/GhostObject.h"
#include "Levels/Level.h"

Ball::LevelEditorCamera::LevelEditorCamera(float fov) : Camera(fov)
{
	m_CanBeSaved = false;
	m_CursorPos = glm::vec3(0, 0, 0);
	m_Distance = 30.0f;
	m_MinDistance = 1.0f;
	m_MaxDistance = 60.0f;
	m_ScrollSpeed = 5.0f;
	m_XSpeed = 2.0f;
	m_YSpeed = 4.0f;
	m_YMinLimit = -20;
	m_YMaxLimit = 80;

	m_CameraX = -0.5f;
	m_CameraY = 0.0f;
}

void Ball::LevelEditorCamera::Init()
{
	// Only initialize the input key binds once
	static bool inputInitialized = false;
	if (!inputInitialized)
	{
		inputInitialized = true;

		auto& input = GetEngine().GetInput();

		if (!input.ContainsAxis("Camera_Forward_Axis"))
		{
			auto& forward = input.CreateAxis("Camera_Forward_Axis");
			forward.AddKeyBind(KEY_S, KEY_W);
			forward.AddStickBind(LEFTY);
			forward.DeadZone(0.15f);
		}

		if (!input.ContainsAxis("Camera_Forward_Axis_Controller"))
		{
			auto& forward = input.CreateAxis("Camera_Forward_Axis_Controller");
			forward.AddStickBind(LEFTY);
			forward.DeadZone(0.15f);
		}

		if (!input.ContainsAxis("Camera_Right_Axis"))
		{
			auto& right = input.CreateAxis("Camera_Right_Axis");
			right.AddKeyBind(KEY_A, KEY_D);
			right.AddStickBind(LEFTX);
			right.DeadZone(0.15f);
		}

		if (!input.ContainsAxis("Camera_Right_Axis_Controller"))
		{
			auto& right = input.CreateAxis("Camera_Right_Axis_Controller");
			right.AddStickBind(LEFTX);
			right.DeadZone(0.15f);
		}

		if (!input.ContainsAxis("Camera_Up_Axis"))
		{
			auto& up = input.CreateAxis("Camera_Up_Axis");
			up.AddStickBind(R2);
			up.AddKeyBind(KEY_Q, KEY_E);
		}

		if (!input.ContainsAxis("Camera_Down_Axis"))
		{
			auto& down = input.CreateAxis("Camera_Down_Axis");
			down.AddStickBind(L2);
		}

		if (!input.ContainsAxis("Camera_Pitch"))
		{
			auto& camPitch = input.CreateAxis("Camera_Pitch");
			camPitch.AddStickBind(RIGHTY);
			camPitch.AddKeyBind(KEY_DOWN, KEY_UP);
			camPitch.DeadZone(0.15f);
		}

		if (!input.ContainsAxis("Camera_Yaw"))
		{
			auto& camYaw = input.CreateAxis("Camera_Yaw");
			camYaw.AddStickBind(RIGHTX);
			camYaw.AddKeyBind(KEY_LEFT, KEY_RIGHT);
			camYaw.DeadZone(0.15f);
		}

		if (!input.ContainsAxis("Mouse_ScrollWheel"))
		{
			auto& camScroll = input.CreateAxis("Mouse_ScrollWheel");
			camScroll.AddStickBind(SCROLL_WHEEL);
			camScroll.AddKeyBind(GAMEPAD_L1, GAMEPAD_R1);
			camScroll.DeadZone(0.15f);
		}
	}
}

void Ball::LevelEditorCamera::Shutdown()
{
}

void Ball::LevelEditorCamera::UpdateMovement(float deltaTime)
{
	auto& input = GetEngine().GetInput();

	// If the control button is held down we do not updated the camera movement
	const bool control = input.GetAction("Control");
	if (control)
		return;

	const auto forwardAxis = input.GetAxis("Camera_Forward_Axis");
	const auto rightAxis = input.GetAxis("Camera_Right_Axis");
	const auto upAxis = input.GetAxis("Camera_Up_Axis");
	const auto downAxis = input.GetAxis("Camera_Down_Axis");

	// Camera movement on the X and Z axis is based on the direction the camera is looking.
	// For the forward axis we only care about the X and Z axis and need to remove the Y axis
	glm::vec3 forward = GetTransform().Forward();
	forward.y = 0;
	forward = glm::normalize(forward);
	forward *= forwardAxis;

	glm::vec3 right = GetTransform().Right() * rightAxis;
	glm::vec3 direction = forward + right;
	// Only normalize the direction if the length of the vector is greater than 1
	if (glm::length(direction) >= 1.0f)
		direction = glm::normalize(direction);
	direction.y += (upAxis - downAxis) * 0.8f;
	m_CursorPos += direction * m_MoveScalar * deltaTime;
	m_Direction = direction;
}

void Ball::LevelEditorCamera::UpdateRotation(float deltaTime)
{
	auto& input = GetEngine().GetInput();
	const auto controllerAxisX = input.GetAxis("Camera_Pitch");
	const auto controllerAxisY = input.GetAxis("Camera_Yaw");
	const auto mouseScroll = input.GetAxis("Mouse_ScrollWheel");
	const glm::vec2 mouseDelta = input.GetMouseDelta();

	// Zoom in and out using the scroll wheel.
	// TODO remove deltatime when we start using the scroll wheel
	m_Distance -= mouseScroll * m_ScrollSpeed * deltaTime;
	m_Distance = glm::clamp(m_Distance, m_MinDistance, m_MaxDistance);

	if (abs(controllerAxisX) > 0 || abs(controllerAxisY) > 0)
	{
		// If we are receiving controller input we take this over the mouse input
		m_CameraX += controllerAxisX * m_XSpeed * m_ViewScalar * deltaTime;
		m_CameraY -= controllerAxisY * m_YSpeed * m_ViewScalar * deltaTime;
	}
	else
	{
		// Handle mouse movements when no controller input is given
		m_CameraX += -mouseDelta.y * m_XSpeed * m_ViewScalar * 0.001f;
		m_CameraY -= mouseDelta.x * m_YSpeed * m_ViewScalar * 0.001f;
	}

	// Clamp the y rotation angle
	// cameraX = ClampAngle(cameraX, yMinLimit, yMaxLimit);
	// Apply the rotation to tha camera transform
	const glm::quat rotation = glm::quat(glm::vec3(m_CameraX, m_CameraY, 0));
	const glm::vec3 position = rotation * glm::vec3(0.0f, 0.0f, m_Distance) + m_CursorPos;
	GetTransform().SetPosition(position);
	GetTransform().SetRotation(rotation);

	// if the distance between the camera and the target is too large we move the camera closer.
	if (glm::abs(m_PrevDistance - m_Distance) > 0.001f)
	{
		m_PrevDistance = m_Distance;
		glm::quat rot = glm::quat(glm::vec3(m_CameraX, m_CameraY, 0));
		glm::vec3 pos = rot * glm::vec3(0.0f, 0.0f, m_Distance) + m_CursorPos;
		GetTransform().SetPosition(pos);
		GetTransform().SetRotation(rot);
	}
}

void Ball::LevelEditorCamera::SetCursorPosition(const glm::vec3& position)
{
	m_CursorPos = position;
}

float Ball::LevelEditorCamera::ClampAngle(float angle, float min, float max) const
{
	constexpr float rad = glm::radians(360.0f);
	if (angle < -rad)
		angle += rad;
	if (angle > rad)
		angle -= rad;
	return glm::clamp(angle, -glm::radians(max), -glm::radians(min));
}