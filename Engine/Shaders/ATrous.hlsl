// Heavily inspired by NVIDIA's Falcor SVGF Pass
// https://github.com/NVIDIAGameWorks/Falcor/blob/master/Source/RenderPasses/SVGFPass/SVGFAtrous.ps.slang

#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/DenoisingStructs.h"

ConstantBuffer<CameraGPU> camera : register(b0);
ConstantBuffer<DenoiseRenderData> renderData : register(b1);

StructuredBuffer<uint> historyBuffer : register(t0);
StructuredBuffer<float> depthBuffer : register(t1);
StructuredBuffer<float3> normalsBuffer : register(t2);
StructuredBuffer<float4> illuminationVariance : register(t3); // float3(illumination) & variance in alpha channel

RWStructuredBuffer<float4> prevIllum : register(u0);

RWStructuredBuffer<float4> aTrousResult : register(u1);

ConstantBuffer<StepSize> stepSize : register(b2);

float GaussianBlur(uint2 id)
{
    float sum = 0.f;
    float kernelSum = 0.f;
    const float kernel[2][2] =
    {
        { 1.0 / 4.0, 1.0 / 8.0 },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };
    
    const int radius = 1;
    
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            const int2 p = id + int2(x, y);
            const bool inside = (p.x >= 0 && p.y >= 0) && (p.x < camera.m_ScreenWidth && p.y < camera.m_ScreenHeight);

            if (inside)
            {
                const float k = kernel[abs(x)][abs(y)];
                kernelSum += k;
                const uint i = p.x + p.y * camera.m_ScreenWidth;
                sum += illuminationVariance[i].a * k;
            }
        }
    }

    return sum / kernelSum;
}

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    
    if (idx.x >= camera.m_ScreenWidth * camera.m_ScreenHeight)
        return;
    
    float screenX = idx.x % camera.m_ScreenWidth;
    float screenY = idx.x / camera.m_ScreenWidth;
    uint2 id = uint2(screenX, screenY);

    float epsVariance = 0.00001f;
    float kernelWeights[3] = { 1.0, 2.0 / 3.0, 1.0 / 6.0 };
    
    const float4 illumination = float4(illuminationVariance[idx.x].rgb, 0.0);
    const float luminance = Luminance(illumination.rgb);
    
    float variance = GaussianBlur(id);
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
   // outputTexture[id] = illuminationVariance[idx.x].a;
   aTrousResult[idx.x] = illuminationVariance[idx.x];
   
    if (depthBuffer[idx.x] >= 0.f)
    {
        float phiIllumination = renderData.m_PhiIllumination * sqrt(abs(variance + epsVariance)) + epsVariance;
        float phiN = renderData.m_PhiNormal;
        float phiDepth = stepSize.m_Value;
        
        float weightedIlluminationSum = 1.0;
        float4 illuminationSum = illumination;
        float weight;
        
        for (int y = -2; y <= 2; y++)
        {
            for (int x = -2; x <= 2; x++)
            {
                if (x == 0 && y == 0)
                    continue;
                
                const int2 p = id + int2(x, y) * stepSize.m_Value;
                const bool inside = (p.x >= 0 && p.y >= 0) && (p.x < camera.m_ScreenWidth && p.y < camera.m_ScreenHeight);
                if (inside)
                {
                    const uint i = p.x + p.y * camera.m_ScreenWidth;
                    const float kernel = kernelWeights[abs(x)] * kernelWeights[abs(y)];
                    const float4 illuminationP = illuminationVariance[i];
                    const float luminanceP = Luminance(illuminationP.rgb);
                    const float depthP = depthBuffer[i];
                    const float3 normalP = normalsBuffer[i];
                    if (depthP >= 0.f)
                    {
                        weight = CalculateWeight(depthBuffer[idx.x], depthP, phiDepth * length(float2(x, y)),
                                                           normalsBuffer[idx.x], normalP, phiN,
                                                            luminance, luminanceP, phiIllumination);
                        
                        const float weightedIllumination = weight * kernel;
                        weightedIlluminationSum += weightedIllumination;
                        illuminationSum += float4(weightedIllumination.xxx, weightedIllumination * weightedIllumination) * illuminationP;
                    }
                }
            }
        }
        aTrousResult[idx.x] = float4(illuminationSum / float4(weightedIlluminationSum.xxx, weightedIlluminationSum * weightedIlluminationSum));
    }
    
    // Write to previous history from the first loop iteration
    if (stepSize.m_Value == 1)
    {
        prevIllum[idx.x] = aTrousResult[idx.x];
    }
    
    
}