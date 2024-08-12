#pragma once
#include "Tools/ToolBase.h"
#include "Tools/ToolManager.h"

namespace Ball
{
	class ToolManager;
	class BindlessHeapViewer : public ToolBase
	{
	public:
		void Init() override
		{
			m_Name = "Bindless heap viewer";
			m_ToolCatagory = ToolCatagory::GRAPHICS;
			m_ToolInterfaceType = ToolInterfaceType::WINDOW;
		}

		void Draw() override;
		void Update() override {}
		void Event() override {}

	private:
		bool m_HideBufferEntires = true;
	};
} // namespace Ball