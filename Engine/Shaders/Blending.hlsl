
#define SHADER_STRUCT 1
#include "ShaderHeaders/GpuModelStruct.h"

struct ScreenDims
{
    uint width;
    uint height;
};
ConstantBuffer<ScreenDims> screenDim : register(b0);

[numthreads(16, 16, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
	// Return if the thread's index is outside the texture bounds
	if (idx.x >= screenDim.width || idx.y >= screenDim.height)
		return;

	RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[RDH_TRANSFER];
    
    // UI texture
	Texture2D<float4> inputTexture = ResourceDescriptorHeap[RDH_ULTRALIGHT];
    SamplerState inputSampler = SamplerDescriptorHeap[LINEAR_CLAMP];
    
    float2 uv = idx.xy / float2(screenDim.width, screenDim.height);
    float4 inputColor = inputTexture.SampleLevel(inputSampler, uv, 0);
    float alpha = inputColor.a + 0.0001;
   outputTexture[idx.xy] =  float4(outputTexture[idx.xy].rgb * (1.f - alpha) + inputColor.bgr * alpha, 1.0);
}
