
Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    
    int width;
    int height;
    outputTexture.GetDimensions(width, height);
    if (idx.x >= width || idx.y >= height)
        return;
    
    float4 neighbours[4];
    int2 coord = idx.xy * 2;
    
    neighbours[0] = inputTexture[coord];
    neighbours[1] = inputTexture[coord + int2(1, 0)];
    neighbours[2] = inputTexture[coord + int2(0, 1)];
    neighbours[3] = inputTexture[coord + int2(1, 1)];
    
    float4 average = (neighbours[0] + neighbours[1] + neighbours[2] + neighbours[3]) * 0.25;
    outputTexture[idx.xy] = average;
}