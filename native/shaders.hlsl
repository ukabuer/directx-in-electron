struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer ShaderParams : register(b0)
{
    float time;
    float width;
    float height;
}

VOut VShader(float4 position : POSITION, float4 color : COLOR)
{
    VOut output;
    output.position = position;
    output.color = color;
    return output;
}

float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
    // return color;

    float2 uv = position.xy / float2(width, height);

    float r = 0.5 + 0.5 * sin(uv.x * 4.0 + uv.y * 2.0 + time * 1.3);
    float g = 0.5 + 0.5 * sin(uv.y * 3.0 - uv.x * 2.5 + time * 1.7 + 2.094);
    float b = 0.5 + 0.5 * cos(uv.x * 2.5 + uv.y * 3.5 - time * 1.1 + 4.189);

    return float4(r, g, b, 1.0);
}
