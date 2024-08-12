#define SHADER_STRUCT 1

#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/GpuModelStruct.h"


#define invPI 0.318309886183f
#define PI 3.141592653589f

struct ScreenDims
{
	uint m_Width;
	uint m_Height;
};

// Expects x and y between 0 and 1 for screen
CamRay GenerateRay(float x, float y, CameraGPU cam)
{
	const float rw = 1.f / float(cam.m_ScreenWidth);
	const float rh = 1.f / float(cam.m_ScreenHeight);
	const float3 dir = normalize(cam.m_ImagePlanePos.xyz + (x * rw) * cam.m_xAxis - (y * rh) * cam.m_yAxis);
	CamRay r;
	r.m_Pos = cam.m_Pos.xyz;
	r.m_Dir = dir;
	return r;
}

uint PackUints(uint a, uint b)
{
	return (a << 16) | b;
}

float3 SRGBToLinear(float3 srgbColor)
{
	// Fast Estimation
	return pow(srgbColor, 2.2);
}

float4 SRGBToLinear(float4 srgbColor)
{
	// Fast Estimation
	return float4(pow(srgbColor.xyz, 2.2), srgbColor.a);
}

float4 Demodulate(float4 sampl, float4 albedo)
{
	float epsilon = 0.001f;
	float4 illumination = sampl / max(float4(epsilon, epsilon, epsilon, epsilon), albedo);
	return illumination;
}

// Calculate Luminance
// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
float Luminance(float3 color)
{
	return dot(color, float3(0.299, 0.587, 0.114));
}

float CalculateWeight(float depthCenter, float depthP, float phiD, float3 normalCenter, float3 normalP, float phiN,
					  float luminanceCenter, float luminanceP, float phiL)
{
	float epsilon = 0.0000001;

	// Depth weight -- TODO possibly calculate depth derivatives
	float difference = abs(depthCenter - depthP);
	float weightDepth = (phiD == 0) ? 0.f
									: difference /
			max(phiD, epsilon); // TODO perhaps this can be multiplied by a value between 2-4 for further improvement

	// Normal weight
	float weightNormal = pow(max(0.f, dot(normalCenter, normalP)), phiN);

	// Luminance weight
	float weightLuminance = abs(luminanceCenter - luminanceP) / phiL; // TODO try different values from 1 - 10

	float weight = exp(-weightDepth - weightLuminance) * weightNormal;
	return weight;
}

