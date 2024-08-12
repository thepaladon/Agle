#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/DenoisingStructs.h"

ConstantBuffer<CameraGPU> camera : register(b0);

RWStructuredBuffer<float4> albedo : register(u0);
StructuredBuffer<float4> emission : register(t0);
StructuredBuffer<float4> aTrousResult : register(t1);

[numthreads(256, 1, 1)]
void main( uint3 idx : SV_DispatchThreadID )
{
    if (idx.x >= camera.m_ScreenWidth * camera.m_ScreenHeight)
        return;
    
    float screenX = idx.x % camera.m_ScreenWidth;
    float screenY = idx.x / camera.m_ScreenWidth;
    uint2 id = uint2(screenX, screenY);
    
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
    
    float4 result = albedo[idx.x] * aTrousResult[idx.x];
    // Write non-noisy emission straight away
    if (length(emission[idx.x]) > 0.f)
    {
        result = emission[idx.x];
    }
    
    outputTexture[id] = float4(result.xyz, 1.0);
    albedo[idx.x] = float4(1.0, 1.0, 1.0, 1.0);
}