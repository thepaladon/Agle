#pragma once

#include <glm/vec3.hpp>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
namespace Ball
{
	struct Line
	{
		glm::vec3 m_Start;
		glm::vec3 m_End;
		glm::vec3 m_Color;
	};

	class Camera;
	class LineDrawer
	{
	public:
		void AddLine(Line line);
		void DrawLines(Camera* cam);
		void Init(uint32_t width, uint32_t height);
		void Shutdown();

	private:
		struct LineData
		{
			glm::mat4 m_MVP;
			glm::vec4 m_Color;
		};
		std::vector<Line> m_Lines;

		// Not used on PS5
		[[maybe_unused]] uint32_t m_MaxLinesNum = 200000;
	};
}; // namespace Ball