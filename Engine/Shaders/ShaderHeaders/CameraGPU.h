#pragma once

#ifndef SHADER_STRUCT
// Math Types
#include <glm/glm.hpp>
typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef uint32_t uint;
#endif

struct CamRay
{
	float3 m_Pos;
	float3 m_Dir;
};

struct GameplaySkyMat
{
	float4x4 m_RotMat;
	uint m_UseSkyMat;
	float m_LightingStrength;
	float m_BackgroundStrength;
};

struct ViewPyramid
{
	float4 m_TopPlane;
	float4 m_BotPlane;
	float4 m_LeftPlane;
	float4 m_RightPlane;
};

struct CameraGPU
{
	float4 m_Pos;
	float4 m_ImagePlanePos;
	float4 m_xAxis;
	float4 m_yAxis;
	uint m_ScreenWidth;
	uint m_ScreenHeight;
	float m_PrimaryConeSpreadAngle;
};
