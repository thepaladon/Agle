#pragma once
#include "Tools/ToolBase.h"

namespace Ball
{
	class ToolManager;
	class AudioParameter : public ToolBase
	{
	public:
		void Init() override
		{
			m_Name = "Audio Parameters";
			m_ToolCatagory = ToolCatagory::AUDIO;
			m_ToolInterfaceType = ToolInterfaceType::WINDOW;
		}
		void Draw() override;
		void Update() override;

	private:
		bool m_Playing = false;
	};
} // namespace Ball