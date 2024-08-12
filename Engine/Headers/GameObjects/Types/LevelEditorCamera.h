#pragma once

#include "GameObjects/Serialization/PrefabReader.h"
#include "GameObjects/Types/Camera.h"

namespace Ball
{
	class LevelEditorCamera : public Camera
	{
	public:
		LevelEditorCamera(float fov = 50.0f);
		~LevelEditorCamera() override = default;

		void Init() override;
		void Shutdown() override;

		void Update(float deltaTime) override {}
		void UpdateMovement(float deltaTime);
		void UpdateRotation(float deltaTime);

		void SetCursorPosition(const glm::vec3& position);
		const glm::vec3& GetCursorPosition() const { return m_CursorPos; }

		const glm::vec3& GetDirection() const { return m_Direction; }

		float m_ViewScalar = 1.0f;
		float m_MoveScalar = 20.0f;

	private:
		float ClampAngle(float angle, float min, float max) const;

		glm::vec3 m_CursorPos;
		float m_Distance;
		float m_MinDistance;
		float m_MaxDistance;
		float m_ScrollSpeed;
		float m_XSpeed;
		float m_YSpeed;
		float m_YMinLimit;
		float m_YMaxLimit;

		float m_CameraX;
		float m_CameraY;
		float m_PrevDistance;

		glm::vec3 m_Direction;

		inline static bool m_InputInitialized = false;

		REFLECT(LevelEditorCamera)
	};

} // namespace Ball