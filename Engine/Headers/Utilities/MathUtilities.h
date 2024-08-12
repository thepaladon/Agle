#pragma once
#include <glm/vec3.hpp>

namespace Ball
{
	namespace Utilities
	{
		/// Linear interpolation of two float values as angles.
		/// Finds shortest path
		/// Returns values between 0 and pi*2 in radians
		///
		/// @param a from float
		/// @param b to float
		/// @param t Interpolation factor. The interpolation is defined in the range [0, 1].
		float AngleLerp(float a, float b, float t);

		/// Linear interpolation of two float values.
		///
		/// @param a from float
		/// @param b to float
		/// @param t Interpolation factor. The interpolation is defined in the range [0, 1].
		float Lerp(float a, float b, float t);

		/// Linear interpolation of two glm::vec3 values.
		///
		/// @param a from glm::vec3
		/// @param b to glm::vec3
		/// @param t Interpolation factor. The interpolation is defined in the range [0, 1].
		glm::vec3 Lerp(glm::vec3 a, glm::vec3 b, float t);

		static inline float Remap(float value, float minIn, float maxIn, float minOut, float maxOut)
		{
			// Map the input value from the range [minIn, maxIn] to the range [minOut, maxOut]
			return minOut + (value - minIn) * (maxOut - minOut) / (maxIn - minIn);
		}

		// Returns the closest aligned value to the input
		uint32_t AlignToClosestUpper(uint32_t input, uint32_t alignment);

	} // namespace Utilities
} // namespace Ball