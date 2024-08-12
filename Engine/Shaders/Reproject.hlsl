#define SHADER_STRUCT 1
#include "Common.hlsl"
#include "ShaderHeaders/CameraGPU.h"
#include "ShaderHeaders/WavefrontStructsGPU.h"

struct ReproSettings
{
	float m_Alpha;
	float m_MomentsAlpha;
	float m_NormalThreshold;
};

ConstantBuffer<ReproSettings> settings : register(b0);

StructuredBuffer<float4> intersectionPoints : register(t0);
StructuredBuffer<float3> currentNormalBuffer : register(t1);
StructuredBuffer<float3> previousNormalBuffer : register(t2);
StructuredBuffer<uint> currentModelPrimID : register(t3);
StructuredBuffer<uint> previousModelPrimID : register(t4);
StructuredBuffer<uint> previousHistory : register(t5);
StructuredBuffer<float2> previousMoments : register(t6);
StructuredBuffer<float4> previousIllumination : register(t7);
StructuredBuffer<float4> frameColor : register(t8);
StructuredBuffer<float4> primaryAlbedo : register(t9);
StructuredBuffer<float4> emission : register(t10);
StructuredBuffer<CameraGPU> cameras : register(t11);

RWStructuredBuffer<uint> historyUpdate : register(u0);
RWStructuredBuffer<float2> currentMoments : register(u1);
RWStructuredBuffer<float4> currentIllumination : register(u2);



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

    // Different geometry
    uint modelPrimIdCur = currentModelPrimID[curIdx];
    uint modelPrimIdPrev = previousModelPrimID[prevIdx];
    uint prevModel = (modelPrimIdPrev >> 16) & 0xFFFF;
    uint prevPrim = modelPrimIdPrev & 0xFFFF;
    uint curModel = (modelPrimIdCur >> 16) & 0xFFFF;
    uint curPrim = modelPrimIdCur & 0xFFFF;

	if (prevModel == 0 || prevPrim == 0 || prevModel != curModel || prevPrim != curPrim)
        return false;

    // Normal deviation
    float thresholdDeviation = settings.m_NormalThreshold;
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
    
    float4 curIllum = Demodulate(frameColor[idx.x] - emission[idx.x], primaryAlbedo[idx.x]);
   
    currentIllumination[idx.x] = float4(curIllum.rgb, 1.f);
    float lum = Luminance(curIllum.rgb);
    // We take 0.5 of the luminance as it is assumed that the previous frame is purely empty black
    // and we have 50% from the new frame and 50% from the previous one
    float2 curMoment = float2(lum, lum * lum) * 0.5f;
    currentMoments[idx.x] = curMoment;
    currentIllumination[idx.x].a = abs(curMoment.g - curMoment.r * curMoment.r);
    
	RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT];
    int prevPixelIdx = (int) intersectionPoints[idx.x].a;
    // We hit the geometry
    if (prevPixelIdx >= 0)
    {
        
        float prevX = float(prevPixelIdx) % currentCamera.m_ScreenWidth;
        float prevY = float(prevPixelIdx) / currentCamera.m_ScreenWidth;
       
        int2 prevPosFloor = int2(prevX, prevY);
        
        // Bilinear check of previous pixels
        // Did we have a single useful pixel from the previous frame, using bilinear 2x2 filter
        bool successBilin = false;
        float4 prevCol = 0.f;
        float prevAccumFrames = 0.f;
        float2 prevMoments = float2(0.f, 0.f);
        
        // A single tap
        if (IsPrevValid(prevPosFloor, idx.x, currentCamera.m_ScreenWidth, currentCamera.m_ScreenHeight))
        {
            uint prevPixelIdx = prevPosFloor.x + prevPosFloor.y * currentCamera.m_ScreenWidth;
            prevCol += previousIllumination[prevPixelIdx];
            prevAccumFrames += (float) previousHistory[prevPixelIdx];
            prevMoments += previousMoments[prevPixelIdx];
            successBilin = true;
        }
        
        // single tap failed -> 1x1 diamond tap bilinear check
        if (!successBilin)
        {
            
            int2 bilinOffset[4] = { int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1) };
            float weightSum = 0.f;
            
            [unroll (4)]
            for (int i = 0; i < 4; i++)
            {
                int2 prevPixel = prevPosFloor + bilinOffset[i];
                if (IsPrevValid(prevPixel, idx.x, currentCamera.m_ScreenWidth, currentCamera.m_ScreenHeight))
                {
                    uint prevPixelIdx = prevPixel.x + prevPixel.y * currentCamera.m_ScreenWidth;
                    prevCol += previousIllumination[prevPixelIdx];
                    prevAccumFrames += (float)previousHistory[prevPixelIdx];
                    prevMoments += previousMoments[prevPixelIdx];
                    weightSum += 1.f;

                }
            }
            
            if (weightSum > 0.f)
            {
                prevCol /= weightSum;
                prevAccumFrames /= weightSum;
                prevMoments /= weightSum;
                successBilin = true;
            }
        }
        
        // 1x1 failed -> do 2x2
        if (!successBilin)
        {
            float weightSum = 0.f;
            
            int2 offsets[8] =
            {
                int2(0, 2),
                int2(0, -2),
                int2(1, 1),
                int2(1, -1),
                int2(-1, 1),
                int2(-1, -1),
                int2(2, 0),
                int2(-2, 0)
            };
            
            [unroll (8)]
            for (int i = 0; i < 8; i++)
            {
                int2 prevPixel = prevPosFloor + offsets[i];
                if (IsPrevValid(prevPixel, idx.x, currentCamera.m_ScreenWidth, currentCamera.m_ScreenHeight))
                {
                    uint prevPixelIdx = prevPixel.x + prevPixel.y * currentCamera.m_ScreenWidth;
                    prevCol += previousIllumination[prevPixelIdx];
                    prevAccumFrames += (float)previousHistory[prevPixelIdx];
                    prevMoments += previousMoments[prevPixelIdx];
                    weightSum += 1.f;
                }
            }
            // 3x3 filter succeeded
            if (weightSum > 0.f)
            {
                prevCol /= weightSum;
                prevAccumFrames /= weightSum;
                prevMoments /= weightSum;
                successBilin = true;
            }
        }
        
        if (successBilin)
        {
            float colorAlpha = max(1.0f / (prevAccumFrames + 1.f), settings.m_Alpha);
            historyUpdate[idx.x] = uint(prevAccumFrames) + 1; //  min(32.f, uint(prevAccumFrames) + 1); 32 =  4 * 8, WHY 8?
            float4 res = colorAlpha * curIllum + (1.f - colorAlpha) * prevCol;
            
            
            float prevLum = Luminance(prevCol.rgb);
            float2 prevMoment = float2(prevLum, prevLum * prevLum);
            
            float momentAlpha = max(1.0f / (prevAccumFrames + 1.f), settings.m_MomentsAlpha);
            float2 moment = momentAlpha * curMoment + (1.f - momentAlpha) * prevMoment;
            currentMoments[idx.x] = moment;
            
            float variance = abs(moment.g - moment.r * moment.r);
            currentIllumination[idx.x] = float4(res.rgb, variance);
            
            return;
        }
       
    }
    historyUpdate[idx.x] = 1;
}