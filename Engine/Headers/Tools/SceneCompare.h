#pragma once
#include "Tools/ToolBase.h"

namespace Ball
{
	class RenderAPI;

	class SceneCompare : public ToolBase
	{
	public:
		void Init() override;
		void Draw() override;
		void Update() override;
		void Event() override;

	private:
		std::string m_StateFileName = "DefaultState.json";
		bool m_LoadOnStart = false;
	};
} // namespace Ball
