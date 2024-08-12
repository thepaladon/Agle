#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/DenoisingStructs.h"
#include "ShaderHeaders/WavefrontStructsGPU.h"

ConstantBuffer<CameraGPU> camera : register(b0);
ConstantBuffer<DenoiseRenderData> renderData : register(b1);
ConstantBuffer<DenoiseDebugSettings> debugSettings : register(b2);

StructuredBuffer<float4> illuminationBuffer : register(t0);
StructuredBuffer<float2> momentsBuffer : register(t1);
StructuredBuffer<uint> historyBuffer : register(t2);
StructuredBuffer<float> depthBuffer : register(t3);
StructuredBuffer<float3> normalsBuffer : register(t4);

RWStructuredBuffer<float4> resultBuffer : register(u0);

ConstantBuffer<History> history : register(b3);

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID )
{
    if (idx.x >= camera.m_ScreenWidth * camera.m_ScreenHeight)
        return;
    
    float screenX = idx.x % camera.m_ScreenWidth;
    float screenY = idx.x / camera.m_ScreenWidth;
    uint2 id = uint2(screenX, screenY);
    
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
    
    float4 illum = illuminationBuffer[idx.x];
    resultBuffer[idx.x] = illum;
    if (historyBuffer[idx.x] < history.m_Value)
    {
        float weightedIllumination = 1.0;
        float3 illuminationSum = illum.rgb;
        float2 momentsSum = momentsBuffer[idx.x];
        
        float phiN = renderData.m_PhiNormal;
        float phiL = renderData.m_PhiIllumination;
        float luminance = Luminance(illum.rgb);
        const int radius = 3;
        if (depthBuffer[idx.x] >= 0.f)
        {
            for (int y = -radius; y <= radius; y++)
            {
                for (int x = -radius; x <= radius; x++)
                {
                    if(x == 0 && y == 0)
                        continue;
                    const int2 p = id + int2(x, y);
                    const bool inside = (p.x >= 0.f && p.y >= 0.f) && (p.x < camera.m_ScreenWidth && p.y < camera.m_ScreenHeight);
                    const uint i = p.x + p.y * camera.m_ScreenWidth;

                    if (inside)
                    {
                        const float3 illuminationP = illuminationBuffer[i];
                        const float luminanceP = Luminance(illuminationP);
                        const float2 momentsP = momentsBuffer[i];
                        const float depthP = depthBuffer[i];
                        const float3 normalP = normalsBuffer[i];
                        float weight = 0.f;
                        if (depthP >= 0.f)
                        {
                           weight = CalculateWeight(depthBuffer[idx.x], depthP, length(float2(x, y)),
                                                            normalsBuffer[idx.x], normalP, phiN,
                                                            luminance, luminanceP, phiL);
                        }
                        weightedIllumination += weight;
                        illuminationSum += illuminationP * weight;
                        momentsSum += momentsP * weight;
                    }
                }
            }
            
            // Clamp sum to >0 to avoid NaNs.
            weightedIllumination = max(weightedIllumination, 0.0001);
            
            illuminationSum /= weightedIllumination;
            momentsSum /= weightedIllumination;
            
            // SO WE BLUR HERE AND THEN IN A-TROUS AS WELL?
            float variance = momentsSum.g - momentsSum.r * momentsSum.r;
            variance *= history.m_Value / historyBuffer[idx.x];
            
            resultBuffer[idx.x] = float4(illuminationSum, variance);
            //resultBuffer[idx.x] = float4(illuminationBuffer[idx.x].rgb, variance);
            //outputTexture[id] = 1.f;
            //return;
        }
    }
   
    //outputTexture[id] = resultBuffer[idx.x].a;
    if (debugSettings.m_VisualizeVariance == 1)
        outputTexture[id] = resultBuffer[idx.x].a;
    if (debugSettings.m_VisualizeWeights == 1)
        outputTexture[id] = float4(resultBuffer[idx.x].rgb, 1.0);
}