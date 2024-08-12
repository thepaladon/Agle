#include "Transform.h"
#include "GameObjects/Serialization/ObjectSerializer.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui.h"

using namespace Ball;

Transform::Transform()
{
	m_Position = glm::vec3(0, 0, 0);
	m_Rotation = glm::vec3(0, 0, 0);
	m_Scale = glm::vec3(1, 1, 1);
	m_Forward = glm::vec3(0, 0, 1);
	m_Right = glm::vec3(1, 0, 0);
	m_Up = glm::vec3(0, 1, 0);
	m_ModelMatrix = glm::mat4(1.0f);
	m_Dirty = false;
}

const glm::mat4& Transform::GetModelMatrix()
{
	if (m_Dirty)
		UpdateTransform();
	return m_ModelMatrix;
}

const glm::mat4 Transform::GetInverseModelMatrix()
{
	if (m_Dirty)
		UpdateTransform();
	return glm::inverse(m_ModelMatrix);
}

const glm::vec3& Transform::SetPosition(float x, float y, float z)
{
	m_Position = glm::vec3(x, y, z);
	m_Dirty = true;
	return m_Position;
}

const glm::vec3& Transform::SetPosition(const glm::vec3& position)
{
	m_Position = position;
	m_Dirty = true;
	return m_Position;
}

const glm::vec3& Transform::Translate(float x, float y, float z)
{
	m_Position += glm::vec3(x, y, z);
	m_Dirty = true;
	return m_Position;
}

const glm::vec3& Transform::Translate(const glm::vec3& translation)
{
	m_Position += translation;
	m_Dirty = true;
	return m_Position;
}

const glm::quat& Transform::SetRotation(const glm::quat& rotation)
{
	m_Rotation = rotation;
	m_Dirty = true;
	return m_Rotation;
}

const glm::quat& Transform::RotateLocal(const glm::quat& rotation)
{
	m_Rotation = m_Rotation * rotation;
	m_Dirty = true;
	return m_Rotation;
}

const glm::quat& Transform::RotateGlobal(const glm::quat& rotation)
{
	m_Rotation = rotation * m_Rotation;
	m_Dirty = true;
	return m_Rotation;
}

const glm::quat& Transform::AngleAxisLocal(float angle, const glm::vec3& axis)
{
	m_Rotation = m_Rotation * glm::angleAxis(angle, axis);
	m_Dirty = true;
	return m_Rotation;
}

const glm::quat& Transform::AngleAxisGlobal(float angle, const glm::vec3& axis)
{
	m_Rotation = glm::angleAxis(angle, axis) * m_Rotation;
	m_Dirty = true;
	return m_Rotation;
}

const glm::vec3& Transform::SetScale(float x, float y, float z)
{
	m_Scale = glm::vec3(x, y, z);
	m_Dirty = true;
	return m_Scale;
}

const glm::vec3& Transform::SetScale(const glm::vec3& scale)
{
	m_Scale = scale;
	m_Dirty = true;
	return m_Scale;
}

const glm::vec3& Transform::GetPosition()
{
	if (m_Dirty)
		UpdateTransform();
	return m_Position;
}

const glm::quat& Transform::GetRotation()
{
	if (m_Dirty)
		UpdateTransform();
	return m_Rotation;
}

const glm::vec3& Transform::GetScale()
{
	if (m_Dirty)
		UpdateTransform();
	return m_Scale;
}

glm::vec3 Transform::SetEulerAngles(float x, float y, float z)
{
	m_Rotation = glm::vec3(x, y, z);
	m_Dirty = true;
	return eulerAngles(m_Rotation);
}

glm::vec3 Transform::SetEulerAngles(const glm::vec3& rotation)
{
	m_Rotation = rotation;
	m_Dirty = true;
	return eulerAngles(m_Rotation);
}

glm::vec3 Transform::GetEulerAngles() const
{
	return eulerAngles(m_Rotation);
}

const glm::vec3& Transform::Forward()
{
	if (m_Dirty)
		UpdateTransform();
	return m_Forward;
}

const glm::vec3& Transform::Right()
{
	if (m_Dirty)
		UpdateTransform();
	return m_Right;
}

const glm::vec3& Transform::Up()
{
	if (m_Dirty)
		UpdateTransform();
	return m_Up;
}

void Transform::UpdateTransform()
{
	// Building model matrix
	const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_Position);
	const glm::mat4 rotationMatrix = glm::toMat4(m_Rotation);
	const glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), m_Scale);
	m_ModelMatrix = translationMatrix * rotationMatrix * scalingMatrix;

	m_Up = glm::normalize(glm::vec3(m_ModelMatrix[1]));
	m_Right = glm::normalize(glm::vec3(m_ModelMatrix[0]));
	m_Forward = glm::normalize(-glm::vec3(m_ModelMatrix[2]));

	// Forward is negative because we're using right-handed coordinate system
	// Up -> Increases as we go up
	// Right -> Increases as we go right
	// Forward -> Decreases as we go forward
	// - Angel [06/03/24]

	m_Dirty = false;
}

void Transform::ImGuiForDebugging()
{
	bool dirty = false;
	ImGui::Begin("Test");
	dirty |= ImGui::DragFloat3("Position", &m_Position.x);

	// Convert quaternion to Euler angles (in degrees)
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(m_Rotation));

	// Display sliders for Euler angles
	if (ImGui::SliderFloat("Yaw", &eulerAngles.y, -180.0f, 180.0f, "%.0f degrees") ||
		ImGui::SliderFloat(
			"Pitch", &eulerAngles.x, -89.0f, 89.0f, "%.0f degrees") || // Limiting pitch to avoid gimbal lock
		ImGui::SliderFloat("Roll", &eulerAngles.z, -180.0f, 180.0f, "%.0f degrees"))
	{
		dirty |= true;
		// Convert updated Euler angles back to quaternion
		m_Rotation = glm::quat(glm::radians(eulerAngles));
	}

	dirty |= ImGui::DragFloat3("Scale", &m_Scale.x);

	if (dirty)
		m_Dirty = true;

	ImGui::End();
}

void Transform::Serialize(SerializeArchive& archive)
{
	archive.Add(ARCHIVE_VAR(m_Position));
	archive.Add(ARCHIVE_VAR(m_Rotation));
	archive.Add(ARCHIVE_VAR(m_Scale));

	if (archive.GetArchiveType() == SerializeArchiveType::LOAD_DATA)
		Dirty();
}
