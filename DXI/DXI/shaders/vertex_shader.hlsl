
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "Common.hlsl"

cbuffer ConstantBuffer : register(b0)
{
    float4x4 WorldViewProjectionMat4x4;
};

PSInput main(VSInput input)
{
    PSInput output;
    //if input.position is not surrounded by float4, then WorldViewProjectionMat4x4 is truncated to float3x4...
    output.position     = mul(float4(input.position, 1.f), WorldViewProjectionMat4x4); 
#if USE_TEXCOORD
    output.tex_coord    = input.tex_coord;
#else
    output.color        = input.color;
#endif
    return output;
}