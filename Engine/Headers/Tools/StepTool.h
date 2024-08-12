#pragma once
#include "Tools/ToolBase.h"

namespace Ball
{
	class ToolManager;
	class StepTool : public ToolBase
	{
	public:
		void Init() override
		{
			m_Name = "Step tool";
			m_ToolCatagory = ToolCatagory::ENGINE;
			m_ToolInterfaceType = ToolInterfaceType::WINDOW;
		}
		void Draw() override;
		void Update() override;

	private:
		bool m_Paused = false;
		float m_PreviousDeltaTime = 0;
		uint32_t m_FrameCount = 0;
		uint8_t m_AdvancedFrameCount = 0;
	};
} // namespace Ball