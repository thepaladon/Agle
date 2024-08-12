#pragma once

// Used for debug render modes
#define RENDER_MODES             \
	ENUM_VALUE(RM_RAY_TRACE, 0)  \
	ENUM_VALUE(RM_ALBEDO, 1)     \
	ENUM_VALUE(RM_NORMAL, 2)     \
	ENUM_VALUE(RM_UV, 3)         \
	ENUM_VALUE(RM_PATH_TRACE, 4) \
	ENUM_VALUE(NOT_IMLPEMENTED, 5)

// Note, you'll have to manually specify
// #define SHADER_STRUCT in every shader (unless using .bsl)
#ifndef SHADER_STRUCT

// an enum of the "RenderModes" for C++ code
#define ENUM_VALUE(name, value) name = value,
enum class RenderModes
{
	RENDER_MODES RenderModeCount
};
#undef ENUM_VALUE

// Math Types
#include <glm/glm.hpp>
typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef glm::vec2 float2;
#else

// An int list of the "RenderModes" for Shader code
#define ENUM_VALUE(name, value) static const int name = value;
RENDER_MODES
#undef ENUM_VALUE
#endif

// ToDo, Make this table dynamic
// Macros for the RDH Headers
#define RDH_TRANSFER 0
#define RDH_SKYBOX 1
#define RDH_ULTRALIGHT 2
#define RDH_TRANSFORMS 3
#define RDH_MODEL_DATA 4

// Not a large fan :(
#define RDH_OUTPUT 5
// 12 RDH_BLOOM_Textures stored from 6 to 17
#define NUM_BLOOM 12
#define RDH_BLUENOISE NUM_BLOOM + RDH_OUTPUT + 1
// HEADER_SIZE determined at runtime
#define RDH_HEADER_SIZE RDH_BLUENOISE + 1

// Bluenoise
#define NUM_BLUENOISE 32

// Macros for the SDH Headers
#define SDH_SKYBOX 9
#define LINEAR_CLAMP 10

struct ModelHeapLocation
{
	int m_ModelStart;
	// Index of the Model in Resource Descriptor
	// m_ModelStart = Primitive Info Buffer
	// m_ModelStart + 1 => Material Info Buffer
	// m_ModelStart + 2 => Buffer Start
	int m_TextureStart; // First Texture this index

#define MATERIAL_OFFSET 1 // Material Info Buffer Offset
#define BUFFER_OFFSET 2 // Buffer Start Offset
};

struct PrimitiveGPU
{
	// Material Index
	int m_MaterialIndex;

	// Accessor Index
	int m_IndexBufferId;
	int m_PositionIndex;
	int m_TexCoordIndex;
	int m_TangentIndex;
	int m_NormalIndex;
	int m_ColorIndex;
	int padding;

	// Note: This gets set during the BLAS creation step from
	// multiplying all Nodes to get into world space from vertex space
	// It exists here because we need it on the GPU.
	float4x4 m_Model;
};

struct MaterialGPU
{
	// Base gltf PBR values
	// 0
	int m_BaseColorTextureIndex;
	int m_BaseColorSamplerIndex;
	int m_MetallicRoughnessTextureIndex;
	int m_MetallicRoughnessSamplerIndex;
	// 16
	float m_MetallicFactor;
	float m_RoughnessFactor;
	int m_NormalTextureIndex;
	int m_NormalSamplerIndex;
	// 32
	float4 m_BaseColorFactor;
	// 48
	// KHR_materials_unlit
	int m_Unlit;
	int m_EmissiveTextureIndex;
	int m_EmissiveSamplerIndex;
	// KHR_materials_emissive_strength
	float m_EmissiveStrength;
	// 64
	float m_NormalTextureScale;
	float3 m_EmissiveFactor;
	// 80
	// KHR_materials_specular
	float m_SpecularFactor;
	int m_SpecularTextureIndex;
	int m_SpecularSamplerIndex;
	int m_SpecularColorTextureIndex;
	// 96
	float3 m_SpecularColorFactor;
	int m_SpecularColorSamplerIndex;
	// 112
	// KHR_materials_transmission
	float m_TransmissionFactor;
	int m_TransmissionTextureIndex;
	int m_TransmissionSamplerIndex;
	// KHR_materials_ior
	float m_Ior;
	// 128
	// KHR_materials_volume
	float3 m_AttenuationColor;
	float m_AttenuationDistance;
	// 144
	float m_TextureDim;
	float m_MaterialLOD;
	float2 m_Padding;
	// 160
};

struct DebugSettings
{
	int m_RenderMode;
};
