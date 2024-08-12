#define SHADER_STRUCT 1

#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/GpuModelStruct.h"
#include "ShaderHeaders/GpuGridStruct.h"

ConstantBuffer<CameraGPU> camera : register(b0);
ConstantBuffer<GridShaderSettings> settings : register(b1);
StructuredBuffer<float> currentDepthBuffer : register(t0);

// Adapted from https://www.shadertoy.com/view/mdVfWw?source=post_page-----727f9278b9d8
float3 pristineGrid(in float2 uv, float2 lineWidth)
{
	float2 val_ddx = ddx(uv);
	float2 val_ddy = ddy(uv);
	float2 uvDeriv = float2(length(float2(val_ddx.x, val_ddy.x)), length(float2(val_ddx.y, val_ddy.y)));
	bool2 invertLine = bool2(lineWidth.x > 0.5, lineWidth.y > 0.5);
	float2 targetWidth =
		float2(invertLine.x ? 1.0 - lineWidth.x : lineWidth.x, invertLine.y ? 1.0 - lineWidth.y : lineWidth.y);
	float2 drawWidth = clamp(targetWidth, uvDeriv, float2(0.5, 0.5));
	float2 lineAA = uvDeriv * 1.5;
	float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
	gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
	gridUV.y = invertLine.y ? gridUV.y : 1.0 - gridUV.y;
	float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);

	grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
	grid2 = lerp(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
	grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
	grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;
	return lerp(grid2.x, 1.0, grid2.y);
}

[numthreads(16, 16, 1)] void main(uint3 idx
								  : SV_DispatchThreadID)
{
	if (idx.x >= camera.m_ScreenWidth || idx.y >= camera.m_ScreenHeight)
		return;

	RWTexture2D<float4> output = ResourceDescriptorHeap[RDH_OUTPUT];

	// raytrace-plane
	CamRay r = GenerateRay(idx.x, idx.y, camera);
	float h = (settings.m_GridOffset.y - r.m_Pos.y) / r.m_Dir.y;
	float3 outval = 0.0;
	float3 test;
	if (h > 0.0)
	{
		uint flattenedIndex = camera.m_ScreenWidth * idx.y + idx.x;
		if (h < currentDepthBuffer[flattenedIndex] || currentDepthBuffer[flattenedIndex] < 1)
		{
			
			float3 pos = r.m_Pos + h * r.m_Dir;
			float2 uv = (pos.xz + settings.m_GridOffset.xz) * (1.0 / settings.m_GridSize);
			float gridVal = pristineGrid(uv, settings.m_LineWidth) * settings.m_Opacity;

			outval = float3(gridVal, gridVal, gridVal);

			//Draws the x and z representing grid directions
			float halfLine = settings.m_LineWidth / 2.0; 
			if (-halfLine < uv.x && uv.x < halfLine)
			{
				outval = float3(1.0, 0.0, 0.0) * settings.m_Opacity;
			}

			if (-halfLine < uv.y && uv.y < halfLine)
			{
				outval = float3(0.0, 0.0, 1.0) * settings.m_Opacity;
			}
		}
	}

	output[idx.xy] += float4(outval, 0.0);
}
