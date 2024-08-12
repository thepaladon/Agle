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

struct AccumFrames
{
	uint m_Enabled;
	uint m_FramesNum;
};

struct ShadeSeedData
{
	uint m_FrameIdx;
	uint m_WavefronLoopIdx;
};

struct DISeedData
{
	uint m_FrameIdx;
	uint m_WavefronLoopIdx;
	uint m_NumLights;
};

struct ThresholdValue
{
	float m_Value;
};

struct ShadeSettings
{
	float m_Threshold;
	float m_ConeSpreadAngle;
	float m_TracingDistanceMultiplier;
};

struct Ray
{
	float3 m_Origin;
	float3 m_Direction;
	float m_ConeWidth;

	float3 m_Throughput; // RGB
	uint m_PixelIdx; // x + width * y

	uint m_LastSpecular; // 1 - last ray was specular, 0 - last ray was regular
	float3 m_Absorption; // for Beer's Law
	float m_MaxT; // for Tracing Distance Selection
};

struct ExtendResult
{
	float2 m_BarycentricUV;
	float m_DistanceT;
	uint m_ModelAndInstanceID;

	uint m_PrimitiveId; // Id of the GLTF primitive
	uint m_TriangleId;
	float2 m_Padding; // ToDo: COMPACT THE VARIABLES
};

struct ShadowRay
{
	float3 m_Origin;
	float m_DistanceT;

	float3 m_Direction;
	uint m_PixelIdx; // x + width * y

	float3 m_Energy; // RGB
	float m_Padding; // ToDo: COMPACT THE VARIABLES
};

struct LightPickData
{
	uint m_ModelId;
	uint m_InstanceId;
	uint m_PrimitiveId;
	uint m_LightsInPrim;
};

struct ReStirSettings
{
	int m_UseReSTIR; // 0 -> Off | 1 -> RIS | 2 -> RIS + Temp | 3 -> Ris + Spatial | 4 -> Ris + Temp + Spatial
	int m_CurrentLightClamp; // clamp( thisFrameEvalLights.M * this, previousFramesEvalLights.M);
	int m_RISRandomLights; // Lights to be evaluated
};

struct ConnectSettings
{
	int m_UseReSTIR; // 0 -> Off | 1 -> RIS | 2 -> RIS + Temp | 3 -> Ris + Spatial | 4 -> Ris + Temp + Spatial
	uint m_WavefronLoopIdx; // clamp( thisFrameEvalLights.M * this, previousFramesEvalLights.M);
	float m_Threshold; // Lights to be evaluated
};
struct Reservoir
{
	uint m_PickedLightIdx; // picked light idx
	float m_Weight; // picked light weight
	float m_WSum; // sum of all weights
	float m_EvalLights; // number of lights processed for this reservoir
	float m_PHat; // length(contrib) of m_PickedLightIdx
	float3 padding;

	// ToDo - Optimization:
	// - Might be worth it converting m_EvalLights to a uint and then
	// packing it with m_PickedLightIdx. But because we can have more that
	// 65535 lights, it might be worth packing it like so:
	// -- 20 bits for m_PickedLightIdx : 1,048,575
	// -- 12 bits for m_EvalLights : 4095
};

struct MaterialHitData
{
	uint m_DiffuseRayID; // used to retrieve the ray that stores data relevant for a DI hit
	float m_AlphaRoughness; // roughness mapped to a more linear change for calculations
	float m_Metallic;
	float m_SpecularWeight;

	float m_Eta;
	float3 m_F0; // full reflectance color (n incidence angle)

	float3 m_BaseColor;
	float m_TransmissionFactor;
};

// Raw Light Data as is encoded in our models
struct LightDataRaw
{
	float3 lightDir;
	float3 lightNormal;
	float distToLight;
	float lightArea;
	float3 lightColor;
};

struct ReproReSTIRSettings
{
	uint m_FrameIdx;
	int m_UseReSTIR; // 0 -> Off | 1 -> TBA | 2 -> RIS, No Temporal, No Spatial
	int m_CurrentLightClamp; // clamp( thisFrameEvalLights.M * this, previousFramesEvalLights.M);
	float m_NormalThreshold;
	float m_DepthThreshold;
};

struct SpatialReSTIRSettings
{
	uint m_FrameIdx;
	int m_UseReSTIR; // 0 -> Off | 1 -> TBA | 2 -> RIS, No Temporal, No Spatial
	int m_NumSamples; // k value from the paper
	float m_NormalThreshold;
	float m_DepthThreshold;
	float m_SpatialRadius;
};