#pragma once

#include "GameObjects/GameObject.h"

namespace Ball
{
	class GhostObject : public GameObject
	{
	public:
		GhostObject();
		~GhostObject() override = default;
		void Init() override;
		void Shutdown() override {}
		void Update(float deltaTime) override;

	private:
		REFLECT(GhostObject)
	};
} // namespace Ball
