#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/WavefrontStructsGPU.h"

struct LineProperties
{
    float4 m_Value; // color and thickness in .a
};

struct SelectedObjectID
{
    uint m_Value;
};

struct OutlineInfo
{
    uint m_SelectedObjectID;
    uint m_OutlineSelected;
    uint m_HoveredObjectID;
    uint m_OutlineHovered;
};

struct HoveredObjectColor
{
    float3 m_Value;
};

ConstantBuffer<ScreenDims> dims : register(b0);
ConstantBuffer<OutlineInfo> outlineInfo : register(b1);
ConstantBuffer<LineProperties> properties : register(b2);
ConstantBuffer<HoveredObjectColor> hoveredColor : register(b3);
StructuredBuffer<uint> instanceIDs : register(t0);

[numthreads(256, 1, 1)]
void main( uint3 idx : SV_DispatchThreadID )
{
    if (idx.x >= dims.m_Width * dims.m_Height)
        return;
    
    float screenX = idx.x % dims.m_Width;
    float screenY = idx.x / dims.m_Width;
    int2 id = int2(screenX, screenY);
    
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
    
    int radius = properties.m_Value.a;
    
    // We want to draw a line if in the blur range there was our and some other model
    bool foundSelected = false;
    bool foundNonSelected = false;
    bool foundHovered = false;
    bool foundNonHovered = false;
    
    for (int x = -radius; x <= radius; x++)
    {
        for (int y = -radius; y <= radius; y++)
        {
            const int2 p = id + int2(x, y);
            const bool inside = all(p >= int2(0, 0)) && all(p < int2(dims.m_Width, dims.m_Height));
            
            if (inside)
            {
                const uint i = p.x + p.y * dims.m_Width;
                uint instanceIdP = instanceIDs[i];
                
                if (instanceIdP == outlineInfo.m_SelectedObjectID)
                    foundSelected = true;
                if (instanceIdP != outlineInfo.m_SelectedObjectID)
                    foundNonSelected = true;
                
                if (instanceIdP == outlineInfo.m_HoveredObjectID)
                    foundHovered = true;
                if (instanceIdP != outlineInfo.m_HoveredObjectID)
                    foundNonHovered = true;
            }
        }
    }
    
    if (foundHovered && foundNonHovered && outlineInfo.m_OutlineHovered == 1)
        outputTexture[id] = float4(hoveredColor.m_Value, 1.0);
    
    if (foundSelected && foundNonSelected && outlineInfo.m_OutlineSelected == 1)
        outputTexture[id] = float4(properties.m_Value.rgb, 1.0);
}