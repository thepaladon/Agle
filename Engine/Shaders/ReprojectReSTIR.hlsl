#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/WavefrontStructsGPU.h"
#include "Random.hlsl"
#include "ReSTIR.hlsl"


ConstantBuffer<ReproReSTIRSettings> settings : register(b0);

StructuredBuffer<CameraGPU> cameras : register(t0);
StructuredBuffer<ViewPyramid> viewPyramid : register(t1);

StructuredBuffer<float> currentDepthBuffer : register(t2);
StructuredBuffer<float> previousDepthBuffer : register(t3);
StructuredBuffer<float3> currentNormalBuffer : register(t4);
StructuredBuffer<float3> previousNormalBuffer : register(t5);
StructuredBuffer<Reservoir> previousReservoirs : register(t6);


RWStructuredBuffer<float4> intersectionPoints : register(u0);
RWStructuredBuffer<Reservoir> currentReservoirs : register(u1);



float DistanceToPlane(float4 plane, float3 pos, out bool inside)
{
    float dotVal = dot(plane.xyz, pos);
    inside = dotVal >= 0.f;
    return abs(dotVal + plane.w) / length(plane.xyz);
}

bool IsPrevValid(int2 prevCoord, uint curIdx, uint width, uint height)
{
    uint prevIdx = prevCoord.x + prevCoord.y * width;
    // Outside of screen boundaries
    if (prevCoord.x < 0 || prevCoord.y < 0 || prevCoord.x >= width || prevCoord.y >= height)
        return false;
    
     // Depth deviation
    float thresholdDeviation = settings.m_DepthThreshold;
    if (previousDepthBuffer[prevIdx] < 0.f)
        return false;
    if (abs(previousDepthBuffer[prevIdx] - currentDepthBuffer[curIdx]) / currentDepthBuffer[curIdx] > thresholdDeviation)
            return false;
    
    // Normal deviation
    thresholdDeviation = settings.m_NormalThreshold;
    if (distance(previousNormalBuffer[prevIdx], currentNormalBuffer[curIdx]) > thresholdDeviation)
        return false;
    return true;
}

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    
    CameraGPU currentCamera = cameras[1];
    
    if (idx.x >= currentCamera.m_ScreenWidth * currentCamera.m_ScreenHeight)
        return;
    
    // Calculate screen coordinates
    float screenX = idx.x % currentCamera.m_ScreenWidth;
    float screenY = idx.x / currentCamera.m_ScreenWidth;
    uint2 id = uint2(screenX, screenY);
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
    //outputTexture[id] = float4(0.f, 0.f, 0.f, 0.f);
    intersectionPoints[idx.x].a = -1.f;
    // We hit the geometry
    if (currentDepthBuffer[idx.x] >= 0.f)
    {
        CameraGPU previousCamera = cameras[0];
        float3 worldSpaceIntersectionPoint = intersectionPoints[idx.x].xyz;
        ViewPyramid previousPyramid = viewPyramid[0];
       
    
        // Determine the position of the current point in the previous frame
        float3 pointInCamSpace = worldSpaceIntersectionPoint - previousCamera.m_Pos.xyz;
       
        bool insideTop = false;
        bool insideBot = false;
        bool insideLeft = false;
        bool insideRight = false;
    
        // Distances between world space intersection point and the 4 view pyramid planes (top, bottom, left, right)
        float distanceToTopPlane = DistanceToPlane(previousPyramid.m_TopPlane, pointInCamSpace, insideTop);
        float distanceToBottomPlane = DistanceToPlane(previousPyramid.m_BotPlane, pointInCamSpace, insideBot);
        float distanceToLeftPlane = DistanceToPlane(previousPyramid.m_LeftPlane, pointInCamSpace, insideLeft);
        float distanceToRightPlane = DistanceToPlane(previousPyramid.m_RightPlane, pointInCamSpace, insideRight);
    
        //Previous Intersection Point
        float x = distanceToLeftPlane * float(currentCamera.m_ScreenWidth) / (distanceToLeftPlane + distanceToRightPlane);
        float y = distanceToTopPlane * float(currentCamera.m_ScreenHeight) / (distanceToTopPlane + distanceToBottomPlane);
    
        float2 previousIntersectionPoint = float2(x, y);
        
        if (!insideRight || !insideLeft || !insideBot || !insideTop)
        {
            intersectionPoints[idx.x].a = -1.f;
            return;
        }
        
        int2 prevPosFloor = int2(previousIntersectionPoint.x, previousIntersectionPoint.y);
        uint prevPixelIdx = prevPosFloor.x + prevPosFloor.y * currentCamera.m_ScreenWidth;
        intersectionPoints[idx.x].a = (float) prevPixelIdx;
        
        if (settings.m_UseReSTIR == 2 || settings.m_UseReSTIR == 4)
        {
            if (IsPrevValid(prevPosFloor, idx.x, currentCamera.m_ScreenWidth, currentCamera.m_ScreenHeight))
            {
                
                Reservoir prevRes = previousReservoirs[prevPixelIdx];
                Reservoir curRes = currentReservoirs[idx.x];
                
				// Clamp previous frame reservoir influence
                prevRes.m_EvalLights =
					min(settings.m_CurrentLightClamp * curRes.m_EvalLights, prevRes.m_EvalLights);

                // Seeding
                uint seed = CombineIntoSeed(settings.m_FrameIdx, idx.x, prevPixelIdx);
                seed = GetWangHashSeed(seed);
                
                // Temporal reuse
                Reservoir tempRisRes = CombineReservoirs(curRes, prevRes, seed);
                currentReservoirs[idx.x] = tempRisRes;
                //outputTexture[id] = tempRisRes.m_Weight; //float4(curRes.m_Weight, prevRes.m_Weight, tempRisRes.m_Weight, 0.f) ;

            }
        }
    }
}