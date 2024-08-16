#pragma once

#include <unordered_set>

#include "Tools/ToolBase.h"

namespace Ball
{

	class Texture;

	class TextureVisualizer : public ToolBase
	{
	public:
		void Init() override;

		void Update() override {}
		void Event() override {}
		void Draw() override;

	private:
		std::unordered_set<Texture*> m_SelectedTextures;
		int m_SelectedFlags = 0; // Variable to hold the selected flags
	};

} // namespace Ball
