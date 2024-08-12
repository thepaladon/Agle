#include "GameObjects/Types/FreeCamera.h"
#include "Engine.h"
#include "Input/Input.h"

Ball::FreeCamera::FreeCamera(float fov) : Camera(fov)
{
	Init();
}

void Ball::FreeCamera::Init()
{
	// Only initialize the input key binds once
	if (!m_InputInitialized)
	{
		m_InputInitialized = true;

		auto& input = GetEngine().GetInput();
		auto& cam_x = input.CreateAxis("Free_Camera_X_Axis");
		cam_x.AddStickBind(LEFTX);
		cam_x.AddKeyBind(KEY_A, KEY_D);
		cam_x.DeadZone(0.15f);

		auto& cam_y = input.CreateAxis("Free_Camera_Y_Axis");
		// L2 and R2 are broken, so this is the alt until then
		cam_y.AddKeyBind(GAMEPAD_R1, GAMEPAD_L1);
		cam_y.AddKeyBind(KEY_F, KEY_R);
		cam_y.DeadZone(0.15f);

		auto& cam_z = input.CreateAxis("Free_Camera_Z_Axis");
		cam_z.AddStickBind(LEFTY);
		cam_z.AddKeyBind(KEY_S, KEY_W);
		cam_z.DeadZone(0.15f);

		auto& cam_pitch = input.CreateAxis("Free_Camera_Pitch");
		cam_pitch.AddStickBind(RIGHTY);
		cam_pitch.AddKeyBind(KEY_DOWN, KEY_UP);
		cam_pitch.DeadZone(0.15f);

		auto& cam_yaw = input.CreateAxis("Free_Camera_Yaw");
		cam_yaw.AddStickBind(RIGHTX);
		cam_yaw.AddKeyBind(KEY_LEFT, KEY_RIGHT);
		cam_yaw.DeadZone(0.15f);
	}
}

void Ball::FreeCamera::Shutdown()
{
}

void Ball::FreeCamera::Update(float deltaTime)
{
	// If this camera is not active we do not update
	if (!IsActiveCamera())
		return;

	// Get input instance and check for camera input values
	auto& input = GetEngine().GetInput();
	const auto cam_x = input.GetAxis("Free_Camera_X_Axis");
	const auto cam_y = input.GetAxis("Free_Camera_Y_Axis");
	const auto cam_z = input.GetAxis("Free_Camera_Z_Axis");
	const auto cam_pitch = input.GetAxis("Free_Camera_Pitch");
	const auto cam_yaw = -input.GetAxis("Free_Camera_Yaw");

	glm::vec3 moveDirection = glm::vec3();
	moveDirection += cam_z * m_Transform.Forward();
	moveDirection += cam_y * glm::vec3(0.0f, 1.0f, 0.0f);
	moveDirection += cam_x * m_Transform.Right();

	if (moveDirection != glm::vec3())
		moveDirection = glm::normalize(moveDirection);
	moveDirection *= m_MoveScalar * deltaTime;

	m_Transform.Translate(moveDirection);

	// Rotate left/right
	m_Transform.AngleAxisGlobal(cam_yaw * m_ViewScalar * deltaTime, glm::vec3(0, 1, 0));

	// Rotate up/down
	m_Transform.AngleAxisLocal(cam_pitch * m_ViewScalar * deltaTime, glm::vec3(1, 0, 0));
}
