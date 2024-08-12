#pragma once
#include "GameObjects/GameObject.h"

namespace Ball
{
	class Cube : public GameObject
	{
	public:
		Cube() {}
		~Cube() override = default;
		void Init() override;
		void Shutdown() override {}
		void Update(float deltaTime) override;
		void LoadTestMeshForCubeGameObject();
		bool m_FirstFrame = true;

		REFLECT(Cube)
	};
} // namespace Ball