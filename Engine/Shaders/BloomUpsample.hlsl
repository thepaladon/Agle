#define SHADER_STRUCT 1
#include "ShaderHeaders/GpuModelStruct.h"
#include "ShaderHeaders/BloomStructsGPU.h"

ConstantBuffer<UpsampleData> upsampleData : register(b0);

[numthreads(16, 16, 1)] void main(uint3 DTid
								  : SV_DispatchThreadID)
{
	float2 xy = DTid.xy * upsampleData.m_TexelSize;

	// Note, this needs to be the same sampler as the one in Perry, Point Sampler
	SamplerState PointSampler = SamplerDescriptorHeap[LINEAR_CLAMP];

	Texture2D<float4> SrcMip = ResourceDescriptorHeap[RDH_OUTPUT + upsampleData.m_MipEvaulating + 1];
	RWTexture2D<float4> Dst = ResourceDescriptorHeap[RDH_OUTPUT + upsampleData.m_MipEvaulating];

	float4 sampled = SrcMip.SampleLevel(PointSampler, (xy - int2(-upsampleData.m_Radius, -upsampleData.m_Radius)) * upsampleData.m_InvSrcDims, 0) * 0.0625f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(0, -upsampleData.m_Radius)) * upsampleData.m_InvSrcDims, 0) * 0.125f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(upsampleData.m_Radius, -upsampleData.m_Radius)) * upsampleData.m_InvSrcDims, 0) * 0.0625f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(-upsampleData.m_Radius, 0)) * upsampleData.m_InvSrcDims, 0) * 0.125f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(0, 0)) * upsampleData.m_InvSrcDims, 0) * 0.25f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(upsampleData.m_Radius, 0)) * upsampleData.m_InvSrcDims, 0) * 0.125f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(-upsampleData.m_Radius, upsampleData.m_Radius)) * upsampleData.m_InvSrcDims, 0) * 0.0625f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(0, upsampleData.m_Radius)) * upsampleData.m_InvSrcDims, 0) * 0.125f;
	sampled += SrcMip.SampleLevel(PointSampler, (xy - int2(upsampleData.m_Radius, upsampleData.m_Radius)) * upsampleData.m_InvSrcDims, 0) * 0.0625f;

	Dst[DTid.xy] += sampled * upsampleData.m_InvMipCount * upsampleData.m_Intensity;
}