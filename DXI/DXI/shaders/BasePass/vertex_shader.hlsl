
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "./Common.hlsl"

#ifdef BASE_PASS_VERTEX_SHADER
    #define VS_INPUT VSInput
    #define PS_INPUT PSInput
#else
    #define VS_INPUT VSInput
    #define PS_INPUT PSInput
#endif

cbuffer ConstantBufferPerObject : register(b0)
{
    float4x4 LocalToWorld;  // Only with per-object translation
    float4x4 WorldToClip;   // Translation * View * Proj (Clip)
};

PSInput main(VSInput input)
{
    PSInput output;
    //if input.position is not surrounded by float4, then WorldViewProjectionMat4x4 is truncated to float3x4...
    output.position             = mul(input.position, WorldToClip); 
    output.color                = input.color;

    float3 world_normal         = mul(float4(input.normal, 1.f), LocalToWorld).xyz;
    output.normal               = world_normal;
    return output;
}