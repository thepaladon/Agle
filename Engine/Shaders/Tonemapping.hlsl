#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/GpuModelStruct.h"
#include "ShaderHeaders/TonemapStructsGPU.h"

// Input and output textures
ConstantBuffer<TonemapParameters> TonemapParametersCB : register(b0);
StructuredBuffer<float4> emission : register(t0);

// Function definitions
float3 Linear(float3 HDR, float max)
{
	float3 SDR = HDR;
	if (max > 0.0)
	{
		SDR = saturate(HDR / max);
	}
	return SDR;
}

float3 Reinhard(float3 HDR, float k)
{
	return HDR / (HDR + k);
}

float3 ReinhardSqr(float3 HDR, float k)
{
	return pow(Reinhard(HDR, k), 2);
}

float3 ACESFilmic(float3 x, float A, float B, float C, float D, float E, float F)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F);
}

[numthreads(16, 16, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
	RWTexture2D<float4> input = ResourceDescriptorHeap[RDH_OUTPUT];
	int width;
	int height;
	input.GetDimensions(width, height);

	if (idx.x >= width || idx.y >= height)
		return; 

	RWTexture2D<float4> output = ResourceDescriptorHeap[RDH_TRANSFER];

	
	int flatID = idx.x + idx.y * width;
	float illum = Luminance(emission[flatID].xyz);

	if (illum > 1.0)
	{
		output[idx.xy] = float4(1.0, 1.0, 1.0, 0.0);
		return;
	}

	float3 HDR = input[idx.xy].xyz;
	HDR = max(0.0, HDR);

	// Apply exposure
	HDR *= exp2(TonemapParametersCB.m_Exposure);

	// Initialize SDR color
	float3 SDR = (float3)0;

	// Select the tonemapping method

	if (TonemapParametersCB.m_TonemapMethod == TM_LINEAR)
	{
		SDR = Linear(HDR, TonemapParametersCB.m_MaxLuminance);
	}
	else if (TonemapParametersCB.m_TonemapMethod == TM_REINHARD)
	{
		SDR = Reinhard(HDR, TonemapParametersCB.m_ReinhardConstant);
	}
	else if (TonemapParametersCB.m_TonemapMethod == TM_REINHARDSQ)
	{
		SDR = ReinhardSqr(HDR, TonemapParametersCB.m_ReinhardConstant);
	}
	else if (TonemapParametersCB.m_TonemapMethod == TM_ACESFILMIC)
	{
		SDR = ACESFilmic(HDR,
						 TonemapParametersCB.m_ShoulderStrength,
						 TonemapParametersCB.m_LinearStrength,
						 TonemapParametersCB.m_LinearAngle,
						 TonemapParametersCB.m_ToeStrength,
						 TonemapParametersCB.m_ToeNumerator,
						 TonemapParametersCB.m_ToeDenominator) /
			ACESFilmic(TonemapParametersCB.m_LinearWhite,
					   TonemapParametersCB.m_ShoulderStrength,
					   TonemapParametersCB.m_LinearStrength,
					   TonemapParametersCB.m_LinearAngle,
					   TonemapParametersCB.m_ToeStrength,
					   TonemapParametersCB.m_ToeNumerator,
					   TonemapParametersCB.m_ToeDenominator);
	}


	// Apply gamma correction and write to the output texture
	output[idx.xy] = float4(pow(abs(SDR), 1.0f / TonemapParametersCB.m_Gamma), 1.0);

	
}