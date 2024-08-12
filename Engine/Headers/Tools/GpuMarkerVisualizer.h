#pragma once

#include "Tools/ToolBase.h"

namespace Ball
{
	class GpuMarkerVisualizer : public ToolBase
	{
	public:
		void Init() override;

		void Update() override {}
		void Event() override {}
		void Draw() override;

	private:
		// Assuming renderer.m_Data is your vector<TimestampData>
		std::map<std::string, std::vector<float>> historyMap;

		float m_ColorSensitivityClamp = 8.f;
	};
} // namespace Ball
