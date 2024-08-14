#pragma once

#include <unordered_set>

#include "Tools/ToolBase.h"

namespace Ball
{

	class Buffer;

	class BufferVisualizer : public ToolBase
	{
	public:
		void Init() override;

		void Update() override {}
		void Event() override {}
		void Draw() override;

	private:
		std::unordered_set<Buffer*> m_SelectedBuffers;
		int m_SelectedFlags = 0; // Variable to hold the selected flags
	};

} // namespace Ball
