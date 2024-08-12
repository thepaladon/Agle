#pragma once
#include "Engine.h"
#include "ShaderHeaders/GpuModelStruct.h"
#include "Tools/ToolBase.h"

namespace Ball
{
	class ToolManager;
	class RenderModeUI : public ToolBase
	{
	public:
		void Init() override;

		void Update() override {}
		void Event() override {}
		void Draw() override;

	private:
		// Current item index. Initialize with the default mode index.
		int m_SelectedRenderMode = 0;

// Macro for automatic creation of the ComboBox
#define ENUM_VALUE(name, value) #name,
		const char* m_RenderModes[(unsigned)RenderModes::RenderModeCount] = {RENDER_MODES};
#undef RENDER_MODE
	};
} // namespace Ball
