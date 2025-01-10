
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

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

cbuffer ConstantBuffer : register(b0)
{
    float4x4 WorldViewProjectionMat4x4;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position     = mul(float4(input.position, 1.f), WorldViewProjectionMat4x4);
    //output.position     = float4(input.position, 1.f);
    output.color        = input.color;
    return output;
}
