
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

// Vertex Shader Input struct
struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

// Vertex Shader Output struct
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

cbuffer SceneConstantBuffer : register(b0)
{
    float4 positionMultiplier;
    float4 colorMultiplier;
};

VSOutput VSMain(VSInput input)
{
    VSOutput result;

    result.position = float4(input.position, 1.0) + positionMultiplier;
    result.color    = input.color * colorMultiplier;

    return result;
}