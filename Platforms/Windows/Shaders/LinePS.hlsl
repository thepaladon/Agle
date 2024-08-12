struct PixelShaderInput
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

struct PixelShaderOutput
{
    float4 Color : SV_Target;
};

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    output.Color = float4(input.Color.rgb, 1.0f);
    return output;
}