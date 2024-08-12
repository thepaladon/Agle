#pragma once
#include "ToolBase.h"

namespace Ball
{
	class InputViewTool : public ToolBase
	{
	public:
		void Init() override;
		void Draw() override;

		void Serialize(SerializeArchive& archive) override;

	private:
		void DrawInputBindings();
		void DrawRawKeyState();
		void DrawDebugInput();
	};
} // namespace Ball