#pragma once
#include "GameObjects/GameObject.h"

namespace Ball
{
	class LevelModel : public GameObject
	{
	public:
		LevelModel() = default;
		~LevelModel() override = default;
		void Init() override;
		void Shutdown() override {}
		void Update(float deltaTime) override;

	private:
		bool m_FirstFrame = true;
		REFLECT(LevelModel)
	};
} // namespace Ball