#pragma once

namespace Ball
{
	class BaseGame
	{
	public:
		BaseGame(){};
		virtual ~BaseGame(){};
		virtual void Initialize() = 0;
		virtual void Update() = 0;
		virtual void OnImGui() = 0;
		virtual void Shutdown() = 0;
	};
} // namespace Ball