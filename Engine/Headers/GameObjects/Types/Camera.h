#pragma once

#include "GameObjects/GameObject.h"
#include "ShaderHeaders/CameraGPU.h"

namespace Ball
{
	class Camera : public GameObject
	{
	public:
		Camera(float fov = 50.0f);
		~Camera() override = default;

		void Init() override;
		void Shutdown() override;

		void Update(float deltaTime) override;

		// SRT class to pass to Compute Shader
		CameraGPU GetGPUCam(uint32_t screenWidth, uint32_t screenHeight);

		glm::mat4 GetProjection();
		glm::mat4 GetView();
		glm::mat4 GetGameplaySkyboxRotMat();

		void SetCameraDir(const glm::vec3& newDir) { m_CameraDirection = newDir; }
		const glm::vec3& GetCameraDir() const { return m_CameraDirection; }

		// Returns true if this is the active camera used for rendering
		bool IsActiveCamera() const { return (this == m_ActiveCamera); }

		// Set the active camera that is used for rendering
		static void SetActiveCamera(Camera* camera) { m_ActiveCamera = camera; }
		// Returns a pointer to the camera used for rendering
		static Camera* GetActiveCamera() { return m_ActiveCamera; }

		float m_NearPlane = 0.01f;
		float m_FarPlane = 1000.f;
		float m_FOV = 50.0f;
		ViewPyramid m_ViewPyramid; // Previous frame's view pyramid

		bool m_IsGameplayCamera = false;

	protected:
		void Serialize(SerializeArchive& archive) override;

		void UpdateCamera(uint32_t screenWidth, uint32_t screenHeight);

		glm::vec3 m_CameraDirection = glm::vec3(0.f);
		glm::vec3 m_ImagePlanePos;
		float m_AspectRatio;

		inline static Camera* m_ActiveCamera = nullptr;

		REFLECT(Camera)
	};
} // namespace Ball