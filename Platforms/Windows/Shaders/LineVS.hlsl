struct Line
{
    matrix MVP;
    float4 Color;
};
StructuredBuffer<Line> LinesSB : register(t0);

struct VertexShaderInput
{
    float3 Position : POSITION;
};
struct VertexShaderOutput
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};
VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    output.Position = mul(LinesSB[instanceID].MVP, float4(input.Position, 1.0f));
    output.Color = LinesSB[instanceID].Color;
    return output;
}