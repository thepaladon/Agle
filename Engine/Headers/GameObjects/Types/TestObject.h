#pragma once
#include "GameObjects/GameObject.h"
struct TestCubeEntity : public Ball::GameObject
{
	TestCubeEntity() : GameObject() {}
	~TestCubeEntity() {}

	void Init() override;

	void Shutdown() override;

	void Update(float deltaTime) override {}
	REFLECT(TestCubeEntity)
};
