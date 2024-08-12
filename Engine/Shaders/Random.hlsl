#define SHADER_STRUCT 1

#include "ShaderHeaders/GpuModelStruct.h"

uint CombineIntoSeed(uint pixelIdx, uint frameIdx, uint wavefrontLoopIdx)
{
    uint combinedValue = (frameIdx * 55001) + (pixelIdx * 78713) + (wavefrontLoopIdx * 26927);
    return combinedValue;
}

uint GetWangHashSeed(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand(inout uint seed) // inout allows to modify the input value, so we get different rand every time
{
    // White Noise
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed / 4294967296.0; // float [0, 1] ( divided on maximum 32-bit unsigned integer) 
}

float3 randf3(inout uint seed)
{
    return float3(rand(seed), rand(seed), rand(seed));
}

float4 SampleBlueNoiseTexture(uint idx, uint frameID)
{
	int screenWidth;
	int screenHeight;
	Texture2D<float4> screenTexture = ResourceDescriptorHeap[RDH_OUTPUT];
	screenTexture.GetDimensions(screenWidth, screenHeight);

	uint screenX = idx % screenWidth;
	uint screenY = idx / screenWidth;

	int textureWidth;
	int textureHeight;
	Texture2D<float4> blueNoiseTexture = ResourceDescriptorHeap[RDH_BLUENOISE];
	blueNoiseTexture.GetDimensions(textureWidth, textureHeight);

	textureWidth = textureWidth - 1;
	textureHeight = textureHeight - 1;
	uint bluenoiseSeed = GetWangHashSeed(frameID / NUM_BLUENOISE);
	uint2 offset = uint2(rand(bluenoiseSeed) * (float)textureWidth, rand(bluenoiseSeed) * (float)textureHeight);
	uint wrappedX = uint(screenX + offset.x) & textureWidth;
	uint wrappedY = uint(screenY + offset.y) & textureHeight;

	return blueNoiseTexture.Load(int3(wrappedX, wrappedY, 0));
}