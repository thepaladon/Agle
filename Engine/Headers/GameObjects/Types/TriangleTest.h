#pragma once
#include "GameObjects/GameObject.h"

namespace Ball
{
	class TriangleTest : public GameObject
	{
	public:
		TriangleTest() {}
		~TriangleTest() override = default;
		void Init() override;
		void Shutdown() override {}
		void Update(float deltaTime) override;

		// Shitty fix ToDo: @Dylano remove later - Angel [05/03/24]
		PhysicsObject* object = nullptr;

		REFLECT(TriangleTest)
	};
} // namespace Ball