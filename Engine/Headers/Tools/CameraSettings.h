#pragma once
#include "Tools/ToolBase.h"

namespace Ball
{
	class ToolManager;
	class CameraSettings : public ToolBase
	{
	public:
		void Init() override
		{
			m_Name = "Camera Settings";
			m_ToolCatagory = ToolCatagory::GRAPHICS;
			m_ToolInterfaceType = ToolInterfaceType::WINDOW;
		}
		void Draw() override;
	};
} // namespace Ball