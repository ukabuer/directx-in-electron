struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer VShaderParams : register(b0)
{
    float time;
}

VOut VShader(float4 position : POSITION, float4 color : COLOR)
{
    VOut output;
    output.position = position + float4(sin(time), 0, 0, 0);
    output.color = color;
    return output;
}

float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
    return color;
}