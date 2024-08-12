#pragma once

// Note, you'll have to manually specify
//  #define SHADER_STRUCT in every shader
#ifndef SHADER_STRUCT

// Math Types
#include <glm/glm.hpp>
typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef glm::vec2 float2;
typedef uint32_t uint;
#endif

#define TM_LINEAR 0
#define TM_REINHARD 1
#define TM_REINHARDSQ 2
#define TM_ACESFILMIC 3

struct TonemapParameters
{
	uint m_TonemapMethod;

	float m_Exposure;
	float m_Gamma;

	// Linear
	float m_MaxLuminance;

	// Reinhard
	float m_ReinhardConstant;

	// ACES Filmic
	float m_ShoulderStrength;
	float m_LinearStrength;
	float m_LinearAngle;
	float m_ToeStrength;
	float m_ToeNumerator;
	float m_ToeDenominator;
	float m_LinearWhite;
};
