struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

Texture2D<float4> SourceColor : register(s0);

VOut VShader(float4 position : POSITION)
{
    VOut output;
    output.position = position;
    return output;
}

float4 PShader(float4 position : SV_POSITION) : SV_TARGET
{
    float4 color = SourceColor.Load(uint3(position.xy, 0));
    return color;
}