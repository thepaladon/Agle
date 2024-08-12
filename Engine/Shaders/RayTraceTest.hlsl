#define SHADER_STRUCT 1

#include "Common.hlsl"

#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/GpuModelStruct.h"

ConstantBuffer<CameraGPU> camera : register(b0);
RaytracingAccelerationStructure sceneBVH : register(t0);
ConstantBuffer<DebugSettings> debug : register(b1);

#define PI 3.14159265359

float4 SampleSky(float3 rayDir, SamplerState samp)
{
	float u = 0.5 + atan2(rayDir.z, rayDir.x) / (2 * PI);
	float v = 0.5 - asin(rayDir.y) / PI;

	Texture2D<float4> skybox = ResourceDescriptorHeap[RDH_SKYBOX];
	return skybox.SampleLevel(samp, float2(u, v), 1.0);
}

[numthreads(16, 16, 1)] void main(uint3 idx
								  : SV_DispatchThreadID)
{
	RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
	StructuredBuffer<float4x4> transforms = ResourceDescriptorHeap[RDH_TRANSFORMS];
	SamplerState mirror = SamplerDescriptorHeap[SDH_SKYBOX];
	float2 uv = idx.xy / float2(camera.m_ScreenWidth, camera.m_ScreenHeight);

	CamRay cameraRay = GenerateRay(idx.x, idx.y, camera);

	RayDesc ray;
	ray.Origin = cameraRay.m_Pos;
	ray.Direction = cameraRay.m_Dir;
	ray.TMin = 0;
	ray.TMax = 100000;

	RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;

	outputTexture[idx.xy] = SampleSky(normalize(ray.Direction), mirror);

	query.TraceRayInline(sceneBVH, RAY_FLAG_NONE, 0xff, ray);

	query.Proceed();

	float3 finalOutput = float3(1.0, 0.0, 1.0);

	if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		float3 albedo = float3(0.0, 0.0, 0.0);
		float2 vertex_uv = float2(0.0, 0.0);
		float3 normal = float3(0.0, 0.0, 0.0);
		float2 attribBary = query.CommittedTriangleBarycentrics();
		uint primitiveIndex = query.CommittedPrimitiveIndex();
		uint geometryIndex = query.CommittedGeometryIndex();
		uint instaceID = query.CommittedInstanceID();
		uint instaceIndex = query.CommittedInstanceIndex();

		float3 barycentrics = float3(1.0 - attribBary.x - attribBary.y, attribBary.x, attribBary.y);

		ModelHeapLocation model_info =
			StructuredBuffer<ModelHeapLocation>(ResourceDescriptorHeap[RDH_MODEL_DATA])[instaceID];
		PrimitiveGPU primitive_info =
			StructuredBuffer<PrimitiveGPU>(ResourceDescriptorHeap[model_info.m_ModelStart])[geometryIndex];

		// Get Indices of Triangle
		uint buff_idx = primitive_info.m_IndexBufferId;
		StructuredBuffer<uint> indexBuffer =
			StructuredBuffer<uint>(ResourceDescriptorHeap[model_info.m_ModelStart + BUFFER_OFFSET + buff_idx]);

		uint tri_v0_id = indexBuffer[(primitiveIndex * 3) + 0];
		uint tri_v1_id = indexBuffer[(primitiveIndex * 3) + 1];
		uint tri_v2_id = indexBuffer[(primitiveIndex * 3) + 2];

		if (primitive_info.m_TexCoordIndex != -1)
		{
			StructuredBuffer<float2> uv_buffer =
				ResourceDescriptorHeap[model_info.m_ModelStart + BUFFER_OFFSET + primitive_info.m_TexCoordIndex];

			vertex_uv = uv_buffer[tri_v0_id] * barycentrics.x + uv_buffer[tri_v1_id] * barycentrics.y +
				uv_buffer[tri_v2_id] * barycentrics.z;
		}
		if (primitive_info.m_MaterialIndex != -1)
		{
			MaterialGPU material_info = StructuredBuffer<MaterialGPU>(
				ResourceDescriptorHeap[model_info.m_ModelStart + MATERIAL_OFFSET])[primitive_info.m_MaterialIndex];

			// BaseColor Texture : Model->m_StartTexture + material->BaseColor;
			if (material_info.m_BaseColorTextureIndex != -1)
			{
				SamplerState samplerAlbedo = SamplerDescriptorHeap[material_info.m_BaseColorSamplerIndex];
				albedo = Texture2D<float3>(
							 ResourceDescriptorHeap[model_info.m_TextureStart + material_info.m_BaseColorTextureIndex])
							 .SampleLevel(samplerAlbedo, vertex_uv, 0.0)
							 .xyz;
			}
			float4x4 transformToWorld = transforms[instaceIndex];
			float4x4 worldMat = mul(transformToWorld, primitive_info.m_Model);

			// Get light normal
			if (primitive_info.m_NormalIndex != -1)
			{
				StructuredBuffer<float3> normalBuffer =
					ResourceDescriptorHeap[model_info.m_ModelStart + BUFFER_OFFSET + primitive_info.m_NormalIndex];
				// Primitive space normal
				normal = normalBuffer[tri_v0_id] * barycentrics.x + normalBuffer[tri_v1_id] * barycentrics.y +
					normalBuffer[tri_v2_id] * barycentrics.z;
				// From primitive to world space
				normal = mul((float3x3)worldMat, normal);
				normal = normalize(normal);
			}

			float3 hitPoint = cameraRay.m_Pos + cameraRay.m_Dir * query.CommittedRayT();

			// Basic Shadows
			RayDesc shadowRay;
			shadowRay.Origin = hitPoint;
			shadowRay.Direction = float3(0.1, 0.8, 0.1);
			shadowRay.TMin = 0.01;
			shadowRay.TMax = 100000;

			RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;

			query.TraceRayInline(sceneBVH, RAY_FLAG_NONE, 0xff, shadowRay);

			query.Proceed();

			if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
			{
				// Illusion of shadow
				finalOutput = albedo * 0.5;
			}
			else
			{
				finalOutput = albedo;
			}
		}

		// Check the output mode and decide what to output
		if (debug.m_RenderMode == RM_ALBEDO)
		{
			outputTexture[idx.xy] = float4(albedo, 1.0);
		}
		else if (debug.m_RenderMode == RM_UV)
		{
			outputTexture[idx.xy] = float4(vertex_uv, 0, 1.0);
		}
		else if (debug.m_RenderMode == RM_NORMAL)
		{
			outputTexture[idx.xy] = float4((normal * 0.5) + 0.5, 1.0);
		}
		else if (debug.m_RenderMode == RM_RAY_TRACE)
		{
			outputTexture[idx.xy] = float4(finalOutput, 1.0);
		}
		else
		{
			// Cool side-streak pattern for non-implemented enms
			bool isCheckerboardBlack = (idx.x % 10) == (idx.y % 10);
			if (isCheckerboardBlack == false)
			{
				outputTexture[idx.xy] = float4(0.0, 0.0, 0.0, 1.0);
			}
			else
			{
				outputTexture[idx.xy] = float4(1.0, 0.0, 1.0, 1.0);
			}
		}
	}
}
