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

struct GridShaderSettings
{
	float m_LineWidth;
	float2 m_GridSize;
	float m_Opacity;
	float3 m_GridOffset;
};
