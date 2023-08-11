
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

// Vertex Shader Input struct
struct VSInput
{
    float3 position : SV_POSITION;
    float4 color : COLOR;
};

// Vertex Shader Output struct
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput result;

    result.position = float4(input.position, 0.0);
    result.color = input.color;

    return result;
}