#pragma once
#include "GameObjects/GameObject.h"

namespace Ball
{
	class Light : public GameObject
	{
	public:
		Light() { SetModel("Models/EmissiveBall/EmissiveBall.gltf"); }
		~Light() override = default;
		void Init() override {}
		void Shutdown() override {}
		void Update(float deltaTime) override { m_Transform.SetPosition(m_Transform.GetPosition()); }

		REFLECT(Light)
	};
} // namespace Ball
