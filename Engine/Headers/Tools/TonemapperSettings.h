#pragma once

#include "Tools/ToolBase.h"

namespace Ball
{
	class TonemapperSettings : public ToolBase
	{
	public:
		void Init() override;

		void Update() override {}
		void Event() override {}
		void Draw() override;

	private:
	};
} // namespace Ball
