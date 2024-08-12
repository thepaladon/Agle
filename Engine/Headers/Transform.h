#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Ball
{
	class SerializeArchive;

	class Transform
	{
		friend class ObjectSerializer;

	public:
		Transform();

		// Gets the model matrix
		const glm::mat4& GetModelMatrix();
		const glm::mat4 GetInverseModelMatrix();

		// Set local position
		const glm::vec3& SetPosition(float x, float y, float z);
		// Set local position
		const glm::vec3& SetPosition(const glm::vec3& position);
		// Move the transform in the given direction and distance of the translation
		const glm::vec3& Translate(float x, float y, float z);
		// Move the transform in the given direction and distance of the translation
		const glm::vec3& Translate(const glm::vec3& translation);

		// Set local rotation
		const glm::quat& SetRotation(const glm::quat& rotation);
		// Rotate the GameObject by the rotation on the local axis
		const glm::quat& RotateLocal(const glm::quat& rotation);
		// Rotate the GameObject by the rotation on the global axis
		const glm::quat& RotateGlobal(const glm::quat& rotation);
		// Rotate the GameObject by the given rotation angle on the local axis
		const glm::quat& AngleAxisLocal(float angle, const glm::vec3& axis);
		// Rotate the GameObject by the given rotation angle on the global axis
		const glm::quat& AngleAxisGlobal(float angle, const glm::vec3& axis);

		// Set the scale of the GameObject
		const glm::vec3& SetScale(float x, float y, float z);
		// Set the scale of the GameObject
		const glm::vec3& SetScale(const glm::vec3& scale);

		// Get the local position
		const glm::vec3& GetPosition();
		// Get the local Rotation
		const glm::quat& GetRotation();
		// Get the local scale
		const glm::vec3& GetScale();

		// Set the rotation from euler angles
		glm::vec3 SetEulerAngles(float x, float y, float z);
		// Set the rotation from euler angles
		glm::vec3 SetEulerAngles(const glm::vec3& rotation);
		// Get the rotation angle as euler angles in radians
		glm::vec3 GetEulerAngles() const;

		// Get a vector that points in the forward direction
		const glm::vec3& Forward();
		// Get a vector that points in the right direction
		const glm::vec3& Right();
		// Get a vector that points in to up direction
		const glm::vec3& Up();

		// Flag the transform as dirty
		void Dirty() { m_Dirty = true; }

		void ImGuiForDebugging();

		void Serialize(SerializeArchive& archive);

	private:
		// Update the transform
		void UpdateTransform();

		// Local transform data
		glm::vec3 m_Position;
		glm::quat m_Rotation;
		glm::vec3 m_Scale;

		glm::vec3 m_Forward; // The forward vector of this transform in world space
		glm::vec3 m_Right; // The right vector of this transform in world space
		glm::vec3 m_Up; // The up vector of this transform in world space

		glm::mat4 m_ModelMatrix; // The model matrix of this transform

		bool m_Dirty; // Has a change been made to this transform
	};

} // namespace Ball
