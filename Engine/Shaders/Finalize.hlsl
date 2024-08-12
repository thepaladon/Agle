#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/WavefrontStructsGPU.h"

ConstantBuffer<ScreenDims> dims : register(b0);
StructuredBuffer<float4> inputColors : register(t0);
ConstantBuffer<AccumFrames> accumFrames : register(b1);
RWStructuredBuffer<float4> currentIllumBuffer : register(u0);
RWStructuredBuffer<uint> currentHistoryBuffer : register(u1);

[numthreads(16, 16, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    if (idx.x >= dims.m_Width || idx.y >= dims.m_Height)
        return;
    // On and Off for frame accumulation
    uint numAccumFrames = accumFrames.m_FramesNum;
 
    uint id = dims.m_Width * idx.y + idx.x;
    
    if (accumFrames.m_Enabled == 1)
    {   
        RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
        
        // Gamma correction + Frame Accumulation
        float3 correctedColor = inputColors[id].rgb / numAccumFrames;

        float4 outColor = float4(correctedColor, 1.0);
        outputTexture[idx.xy] = outColor;
        
        currentIllumBuffer[id] = float4(inputColors[id].rgb / numAccumFrames, 1.0);
        currentHistoryBuffer[id] = numAccumFrames;
        return;
    }
    currentIllumBuffer[id] = inputColors[id];
}