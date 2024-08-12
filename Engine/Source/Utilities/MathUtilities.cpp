#include "Utilities/MathUtilities.h"

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace Ball
{
	namespace Utilities
	{
		float AngleLerp(float a, float b, float t)
		{
			float difference = glm::abs(a - b);
			if (difference > glm::pi<float>())
			{
				// We need to add on to one of the values.
				if (b > a)
				{
					// We'll add it on to start...
					a += glm::pi<float>() * 2;
				}
				else
				{
					// Add it on to end.
					b += glm::pi<float>() * 2;
				}
			}

			// Interpolate it.
			float value = (a + ((b - a) * t));

			// Wrap it..
			float rangeZero = glm::pi<float>() * 2;

			if (value >= 0 && value <= (glm::pi<float>() * 2))
				return value;

			return std::fmod(value, rangeZero);
		}

		float Lerp(float a, float b, float t)
		{
			float timeStep = glm::clamp(t, 0.0f, 1.0f);
			return a + timeStep * (b - a);
		}

		glm::vec3 Lerp(glm::vec3 a, glm::vec3 b, float t)
		{
			float timeStep = glm::clamp(t, 0.0f, 1.0f);
			return a + timeStep * (b - a);
		}

		uint32_t AlignToClosestUpper(uint32_t input, uint32_t alignment)
		{
			if (input % alignment == 0)
				return input;
			else
				return ((input / alignment) + 1) * alignment;
		}
	} // namespace Utilities
} // namespace Ball