
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

// Vertex Shader Input struct
struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

// Vertex Shader Output struct
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Constant buffer
//cbuffer ConstantBuffer : register(b0)
//{
//    float4 colorMultiplier;
//};
//
struct CB
{
    float4 colorMultiplier;
};
ConstantBuffer<CB> cbObj : register(b0);

VSOutput VSMain(VSInput input)
{
    VSOutput result;

    result.position = float4(input.position, 0.0);
    result.color = input.color * cbObj.colorMultiplier;

    return result;
}