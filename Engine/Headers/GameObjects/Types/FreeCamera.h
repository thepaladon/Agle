#pragma once

#include "GameObjects/Types/Camera.h"

namespace Ball
{
	class FreeCamera : public Camera
	{
	public:
		FreeCamera(float fov = 50.0f);
		~FreeCamera() override = default;

		void Init() override;
		void Shutdown() override;

		void Update(float deltaTime) override;

		float m_ViewScalar = 0.8f;
		float m_MoveScalar = 2.0f;

	private:
		inline static bool m_InputInitialized = false;

		REFLECT(FreeCamera)
	};

} // namespace Ball