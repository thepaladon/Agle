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
typedef uint16_t uint16;
#endif

#define WIDTH_HEIGHT_EVEN 0 // Both the width and the height of the texture are even.
#define WIDTH_ODD_HEIGHT_EVEN 1 // The texture width is odd and the height is even.
#define WIDTH_EVEN_HEIGHT_ODD 2 // The texture width is even and teh height is odd.
#define WIDTH_HEIGHT_ODD 3 // Both the width and height of the texture are odd.

struct DownsampleData
{
	uint m_SrcDimension; // Width and height of the source texture are even or odd.
	float2 m_TexelSize; // 1.0 / OutMip1.Dimensions
	uint m_UseKaris13Fetch; // Use keris average to preserve luminance and fetch 13 samples to blur
	uint m_MipEvaulating; // Mip level we're downsampling to (Target Mip)
};

struct UpsampleData
{
	float2 m_TexelSize;
	float2 m_InvSrcDims;
	uint m_Radius;
	uint m_MipEvaulating; // Mip level we're upsampling to (Target Mip)
	float m_Intensity;
	float m_InvMipCount;
};