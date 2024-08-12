#define SHADER_STRUCT 1
#include "ShaderHeaders/GpuModelStruct.h"
#include "ShaderHeaders/BloomStructsGPU.h"

ConstantBuffer<DownsampleData> downsampleData : register(b0);


float Max3(float3 x)
{
    return max(x.x, max(x.y, x.z));
}

bool IsNaN(float x)
{
	return (asuint(x) & 0x7fffffff) > 0x7f800000;
}

float4 Sample(int2 xy)
{
    // Note: This needs to be the same sampler as in Rendere.h of Perry
	SamplerState LinearClampSampler = SamplerDescriptorHeap[LINEAR_CLAMP];
	Texture2D<float4> inputTexture = ResourceDescriptorHeap[RDH_OUTPUT + downsampleData.m_MipEvaulating - 1];

    if (downsampleData.m_UseKaris13Fetch == 1)
    {
        float2 UV1 = downsampleData.m_TexelSize * (xy + float2(0.25, 0.25));
        float2 Off = downsampleData.m_TexelSize * 0.5;
        
        float4 sample1 = inputTexture.SampleLevel(LinearClampSampler, UV1, 0);
        float4 sample2 = inputTexture.SampleLevel(LinearClampSampler, UV1 + float2(Off.x, 0.0)  , 0);
        float4 sample3 = inputTexture.SampleLevel(LinearClampSampler, UV1 + float2(0.0, Off.y)  , 0);
        float4 sample4 = inputTexture.SampleLevel(LinearClampSampler, UV1 + float2(Off.x, Off.y), 0);

        // Why DOES this FIX the flickering? 
        // It's almost like the GPU realizes that you're searching for a NaN and doesn't throw it at all 
        // DO NOT FIX THIS "IMPLICIT YPE NARROWING" WARNING, IT CAUSE THE NaN issue to re-appear?!?
        if (IsNaN(sample1)) sample1 = 1.0f;
        if (IsNaN(sample2)) sample2 = 1.0f;
        if (IsNaN(sample3)) sample3 = 1.0f;
        if (IsNaN(sample4)) sample4 = 1.0f;

        float w1 = 1.0f / (Max3(sample1.rgb) + 0.001f);
        float w2 = 1.0f / (Max3(sample2.rgb) + 0.001f);
        float w3 = 1.0f / (Max3(sample3.rgb) + 0.001f);
        float w4 = 1.0f / (Max3(sample4.rgb) + 0.001f);

        float totalWeight = 1.0f / (w1 + w2 + w3 + w4 + 0.001f);

        return (sample1 * w1 +
            sample2 * w2 +
            sample3 * w3 +
            sample4 * w4) * totalWeight;
    }
    else
    {
		if (downsampleData.m_SrcDimension == WIDTH_HEIGHT_EVEN)
		{
			float2 UV = downsampleData.m_TexelSize * (xy + 0.5);
			return inputTexture.SampleLevel(LinearClampSampler, UV, 0);
		}
		else if (downsampleData.m_SrcDimension == WIDTH_ODD_HEIGHT_EVEN)
		{
			// > 2:1 in X dimension
			// Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x horizontally.
			float2 UV1 = downsampleData.m_TexelSize * (xy + float2(0.25, 0.5));
			float2 Off = downsampleData.m_TexelSize * float2(0.5, 0.0);

			return 0.5 *
				(inputTexture.SampleLevel(LinearClampSampler, UV1, 0) +
				 inputTexture.SampleLevel(LinearClampSampler, UV1 + Off, 0));
		}
		else if (downsampleData.m_SrcDimension == WIDTH_EVEN_HEIGHT_ODD)
		{
			// > 2:1 in Y dimension
			// Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x vertically.
			float2 UV1 = downsampleData.m_TexelSize * (xy + float2(0.5, 0.25));
			float2 Off = downsampleData.m_TexelSize * float2(0.0, 0.5);

			return 0.5 *
				(inputTexture.SampleLevel(LinearClampSampler, UV1, 0) +
				 inputTexture.SampleLevel(LinearClampSampler, UV1 + Off, 0));
		}
		else if (downsampleData.m_SrcDimension == WIDTH_HEIGHT_ODD)
		{
			// > 2:1 in both dimensions
			// Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x in both
			// directions.
			float2 UV1 = downsampleData.m_TexelSize * (xy + float2(0.25, 0.25));
			float2 Off = downsampleData.m_TexelSize * 0.5;

			float4 sampled = inputTexture.SampleLevel(LinearClampSampler, UV1, 0);
			sampled += inputTexture.SampleLevel(LinearClampSampler, UV1 + float2(Off.x, 0.0), 0);
			sampled += inputTexture.SampleLevel(LinearClampSampler, UV1 + float2(0.0, Off.y), 0);
			sampled += inputTexture.SampleLevel(LinearClampSampler, UV1 + float2(Off.x, Off.y), 0);
			sampled *= 0.25;

			return sampled;
		}
    }

    return (float4)0;
}

[numthreads(16, 16, 1)] void main(uint3 DTid
								  : SV_DispatchThreadID)
{
    // - is LEFT UP, + is DOWN RIGHT
    int2 xy = DTid.xy;

    // Outer squares
    float4 s000 = Sample(xy + int2(-2, -2));
    float4 s010 = Sample(xy + int2(-2, 0));
    float4 s020 = Sample(xy + int2(-2, 2));
    float4 s100 = Sample(xy + int2(0, -2));
    float4 s110 = Sample(xy + int2(0, 0));
    float4 s120 = Sample(xy + int2(0, 2));
    float4 s200 = Sample(xy + int2(2, -2));
    float4 s210 = Sample(xy + int2(2, 0));
    float4 s220 = Sample(xy + int2(2, 2));

    // Inner square
    float4 s001 = Sample(xy + int2(-1, -1));
    float4 s002 = Sample(xy + int2(1, -1));
    float4 s003 = Sample(xy + int2(-1, 1));
    float4 s004 = Sample(xy + int2(1, 1));

    float4 s10 = (s000 + s100 + s010 + s110) * 0.25f * 0.125f;
    float4 s20 = (s010 + s110 + s020 + s120) * 0.25f * 0.125f;
    float4 s01 = (s100 + s200 + s110 + s210) * 0.25f * 0.125f;
    float4 s02 = (s110 + s210 + s120 + s220) * 0.25f * 0.125f;

    float4 sInner = (s001 + s002 + s003 + s004) * 0.25f * 0.5f;


    float4 Src1 = s10 + s20 + s01 + s02 + sInner;

    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_OUTPUT + downsampleData.m_MipEvaulating];
    outputTexture[DTid.xy] = Src1;
}