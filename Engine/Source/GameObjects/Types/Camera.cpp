#include "GameObjects/Types/Camera.h"
#include "GameObjects/Serialization/ObjectSerializer.h"
#include "Engine.h"
#include "Window.h"

float4 CalculatePlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
	glm::vec3 normal = glm::cross(a, b);
	if (glm::dot(normal, c) < 0.f)
		normal = -normal;
	float d = -glm::dot(glm::vec3(0.f), a);
	return float4(normal, d);
}

Ball::Camera::Camera(float fov) : m_FOV(fov)
{
	m_AspectRatio =
		static_cast<float>(Ball::GetWindow().GetWidth()) / static_cast<float>(Ball::GetWindow().GetHeight());
}

void Ball::Camera::Init()
{
}

void Ball::Camera::Shutdown()
{
}

void Ball::Camera::Update(float deltaTime)
{
}

void Ball::Camera::Serialize(SerializeArchive& archive)
{
	archive.Add(ARCHIVE_VAR(m_FOV));
	if (archive.GetArchiveType() == SerializeArchiveType::LOAD_DATA)
		SetActiveCamera(this);
}

glm::mat4 Ball::Camera::GetGameplaySkyboxRotMat()
{
	// Get angle from velocity input
	const glm::vec3 v1 = glm::normalize(m_Transform.Forward() * glm::vec3(1, 0, 1));
	const glm::vec3 v2 = glm::vec3(0, 1, 0);

	// Calculate angle between two vectors (does not support negative numbers)
	float yaw = atan2(v1.z, v1.x) - atan2(v2.z, v2.x) + glm::pi<float>() / 2.f;

	// Copy camera matrix and rotate around y axis to make it stationairy around Y
	glm::mat4 m = m_Transform.GetInverseModelMatrix();
	m = glm::rotate(m, yaw, -m_Transform.Up());
	m = glm::rotate(m, glm::radians(17.5f), -m_Transform.Right());
	return m;
}

void Ball::Camera::UpdateCamera(uint32_t screenWidth, uint32_t screenHeight)
{
	m_AspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

	const glm::vec3 viewDirection = m_Transform.Forward();
	const glm::vec3 right = m_Transform.Right();
	const glm::vec3 up = m_Transform.Up();

	const float left = m_AspectRatio * 0.5f;
	constexpr float top = 0.5f;
	const float m_Dist = 0.5f / tan(m_FOV * glm::pi<float>() / 360.f);
	m_ImagePlanePos = m_Dist * viewDirection - left * right + top * up;

	// Calculate direction vectors for the view pyramid
	const glm::vec3 topLeft = normalize(m_ImagePlanePos);
	const glm::vec3 topRight = normalize(m_ImagePlanePos + right * m_AspectRatio);
	const glm::vec3 bottomLeft = normalize(m_ImagePlanePos - up);
	const glm::vec3 bottomRight = normalize(m_ImagePlanePos + right * m_AspectRatio - up);

	// Calculate planes values
	// Calculate the top plane
	m_ViewPyramid.m_TopPlane = CalculatePlane(topLeft, topRight, -up);

	// Calculate the bottom plane
	m_ViewPyramid.m_BotPlane = CalculatePlane(bottomLeft, bottomRight, up);

	// Calculate the left plane
	m_ViewPyramid.m_LeftPlane = CalculatePlane(topLeft, bottomLeft, right);

	// Calculate the right plane
	m_ViewPyramid.m_RightPlane = CalculatePlane(topRight, bottomRight, -right);
}

glm::mat4 Ball::Camera::GetProjection()
{
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
	return projectionMatrix;
}

glm::mat4 Ball::Camera::GetView()
{
	return m_Transform.GetInverseModelMatrix();
}

CameraGPU Ball::Camera::GetGPUCam(uint32_t screenWidth, uint32_t screenHeight)
{
	UpdateCamera(screenWidth, screenHeight);
	const glm::vec3 xAxis = m_Transform.Right() * m_AspectRatio;
	const glm::vec3 yAxis = m_Transform.Up();

	auto gpu_cam = CameraGPU();
	gpu_cam.m_ImagePlanePos = float4(m_ImagePlanePos.x, m_ImagePlanePos.y, m_ImagePlanePos.z, 1.0);
	gpu_cam.m_Pos = float4(m_Transform.GetPosition(), 1.0);
	gpu_cam.m_xAxis = float4(xAxis.x, xAxis.y, xAxis.z, 1.0);
	gpu_cam.m_yAxis = float4(yAxis.x, yAxis.y, yAxis.z, 1.0);
	gpu_cam.m_ScreenHeight = screenHeight;
	gpu_cam.m_ScreenWidth = screenWidth;
	gpu_cam.m_PrimaryConeSpreadAngle = glm::radians(m_FOV / float(screenHeight));
	return gpu_cam;
}