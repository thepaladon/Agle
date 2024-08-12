#pragma once

#ifndef SHADER_STRUCT
// Math Types
#include <glm/glm.hpp>
typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef uint32_t uint;
#endif

struct DenoiseRenderData
{
	float m_PhiNormal;
	float m_PhiIllumination;
};

struct StepSize
{
	int m_Value;
};

struct History
{
	int m_Value;
};

struct DenoiseDebugSettings
{
	uint m_DemodulateIndirectLighting;
	uint m_DemodulateDirectLighting;
	uint m_VisualizeVariance;
	uint m_VisualizeWeights;
};