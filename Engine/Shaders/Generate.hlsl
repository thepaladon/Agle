#define SHADER_STRUCT 1

#include "Common.hlsl"
#include "Random.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/WavefrontStructsGPU.h"

ConstantBuffer<CameraGPU> camera : register(b0);
ConstantBuffer<AccumFrames> accumFrames : register(b1);

RWStructuredBuffer<Ray> rayBatch : register(u0);
RWStructuredBuffer<uint> rayCount : register(u1);
RWStructuredBuffer<float4> wavefrontOutput : register(u2);

// Reprojection reset
RWStructuredBuffer<float3> currentNormal : register(u3);
RWStructuredBuffer<float3> previousNormal : register(u4);
RWStructuredBuffer<uint> currentId : register(u5);
RWStructuredBuffer<uint> previousId : register(u6);
RWStructuredBuffer<uint> currentHistory : register(u7);
RWStructuredBuffer<uint> previousHistory : register(u8);
RWStructuredBuffer<float2> currentMoments : register(u9);
RWStructuredBuffer<float2> previousMoments : register(u10);
RWStructuredBuffer<float4> currentIllumination : register(u11);

RWStructuredBuffer<float4> emission : register(u12);
RWStructuredBuffer<float4> filterResult : register(u13);

[numthreads(16, 16, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{   
    if (idx.x >= camera.m_ScreenWidth || idx.y >= camera.m_ScreenHeight)
        return; 

    uint pixelIdx = camera.m_ScreenWidth * idx.y + idx.x;
    uint seed = CombineIntoSeed(pixelIdx, accumFrames.m_FramesNum, 0);
    seed = GetWangHashSeed(seed);
    
    // Generate rays with AA
    CamRay camRay = GenerateRay(float(idx.x) + rand(seed), float(idx.y) + rand(seed), camera); //  + 0.5 to disable jittering
    rayBatch[pixelIdx].m_Origin = camRay.m_Pos;
    rayBatch[pixelIdx].m_Direction = camRay.m_Dir;
    rayBatch[pixelIdx].m_PixelIdx = pixelIdx;
    rayBatch[pixelIdx].m_Throughput = float3(1.f, 1.f, 1.f);
    rayBatch[pixelIdx].m_LastSpecular = 1; // First is always considered specular, as we need to render light sources
    rayBatch[pixelIdx].m_Absorption = float3(0.f, 0.f, 0.f);
    rayBatch[pixelIdx].m_ConeWidth = 0.f;
    rayBatch[pixelIdx].m_MaxT = 100000.f;
    
    //wavefrontOutput[pixelIdx] = float4((camRay.m_Dir + 1.f) * 0.5f, 0.f);
    //return;
    // Number of generated rays with 1 spp 
    rayCount[0] = camera.m_ScreenWidth * camera.m_ScreenHeight;
    
    // Refresh frame energy data, if we don't accumulate frames
    if (accumFrames.m_Enabled == 0)
    {
        wavefrontOutput[pixelIdx] = float4(0.f, 0.f, 0.f, 0.f);
    }
    
    // Write to previous frame buffers
    previousNormal[pixelIdx] = currentNormal[pixelIdx];
    previousId[pixelIdx] = currentId[pixelIdx];
    previousHistory[pixelIdx] = currentHistory[pixelIdx];

    previousMoments[pixelIdx] = currentMoments[pixelIdx];
    
    // Reset current frame buffers
    currentNormal[pixelIdx] = float3(0.f, 0.f, -100.f);
    currentId[pixelIdx] = 0;
    currentHistory[pixelIdx] = 0;
    currentMoments[pixelIdx] = float2(0.f, 0.f);
    currentIllumination[pixelIdx] = float4(0.f, 0.f, 0.f, 0.f);
    emission[pixelIdx] = float4(0.f, 0.f, 0.f, 0.f);
    filterResult[pixelIdx] = float4(0.f, 0.f, 0.f, 0.f);
}