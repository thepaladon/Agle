#pragma once
#include "BaseGame.h"

class GraphicsScene : public Ball::BaseGame
{
public:
	GraphicsScene(){};
	void Initialize() override;
	void Update() override;
	void OnImGui() override{};
	void Shutdown() override;
	~GraphicsScene() override;
};
