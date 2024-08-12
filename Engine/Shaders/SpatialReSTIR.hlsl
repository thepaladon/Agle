#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "Random.hlsl"
#include "ReSTIR.hlsl"


ConstantBuffer<SpatialReSTIRSettings> settings : register(b0);

StructuredBuffer<float> depthBuffer : register(t0);
StructuredBuffer<float3> normalBuffer : register(t1);
StructuredBuffer<CameraGPU> cameras : register(t2);
StructuredBuffer<Reservoir> currentReservoirs : register(t3);
StructuredBuffer<uint> RayCount : register(t4);

RWStructuredBuffer<Reservoir> previousReservoirs : register(u0);

bool IsSampValid(int2 sampIdx, uint curIdx, uint width, uint height)
{
    uint sampleIdx = sampIdx.x + sampIdx.y * width;
    // Outside of screen boundaries
    if (sampIdx.x < 0 || sampIdx.y < 0 || sampIdx.x >= width || sampIdx.y >= height)
        return false;
    
     // Depth deviation
    if (depthBuffer[sampleIdx] < 0.f)
        return false;
    if (abs(depthBuffer[sampleIdx] - depthBuffer[curIdx]) / depthBuffer[curIdx] > settings.m_DepthThreshold)
            return false;
    
    // Normal deviation
    if (distance(normalBuffer[sampleIdx], normalBuffer[curIdx]) > settings.m_NormalThreshold)
        return false;
    return true;
}

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    CameraGPU currentCamera = cameras[1];
    if (idx.x >= currentCamera.m_ScreenWidth * currentCamera.m_ScreenHeight)
        return;
    
    if (depthBuffer[idx.x] >= 0.f)
    {
       
        if (settings.m_UseReSTIR == 3 || settings.m_UseReSTIR == 4)
        {
            // Calculate screen coordinates
            float screenX = idx.x % currentCamera.m_ScreenWidth;
            float screenY = idx.x / currentCamera.m_ScreenWidth;
            uint2 id = uint2(screenX, screenY);
    
            // Seeding
            uint seed = CombineIntoSeed(settings.m_FrameIdx, idx.x, 0);
            seed = GetWangHashSeed(seed);
            Reservoir curRes = currentReservoirs[idx.x];
                
            for (int i = 0; i < settings.m_NumSamples; i++)
            {
        
            // Get a random sample within the radius and texture bounds
            // ToDo: uniform distribution with Blue Noise
                int2 sampleId = int2(screenX + (rand(seed) - 0.5f) * 2.f * settings.m_SpatialRadius,
                                     screenY + (rand(seed) - 0.5f) * 2.f * settings.m_SpatialRadius);
        
                if (IsSampValid(sampleId, idx.x, currentCamera.m_ScreenWidth, currentCamera.m_ScreenHeight))
                {
                    uint sampleIdx = currentCamera.m_ScreenWidth * sampleId.y + sampleId.x;
                    Reservoir sampRes = currentReservoirs[sampleIdx];
                    // Spatial reuse
                    curRes = CombineReservoirs(curRes, sampRes, seed);
                }
            }
            // Write our final reservoir to the history buffer
            previousReservoirs[idx.x] = curRes;
            return;
        }
    }
    // Update history with temporal result if no spatial is enabled
    // We also clear history if the current sample hits sky
    previousReservoirs[idx.x] = currentReservoirs[idx.x];
}