
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "../Common.hlsl"

cbuffer ConstantBufferPerObject : register(b0)
{
    float4x4 LocalToWorld;          // Only with per-object translation
    float4x4 WorldToClip;           // Translation * View * Proj (Clip)
    float4x4 LocalToLightClip;      // Translation * Light View * Light Proj | ShadowPass uses it
    float4x4 LocalToWorldNormal;    // Inverse transpose of LocalToWorld, see D3D12Math.h
};

PSInput main(VSInput input)
{
    PSInput output;

    //if input.position is not surrounded by float4, then WorldViewProjectionMat4x4 is truncated to float3x4...
    output.position             = mul(input.position, WorldToClip);
    output.tex_coord            = input.tex_coord;

    // The normal gets its own matrix, LocalToWorld would shear it under a non uniform
    // scale. w of 0 keeps the translation out of it, a normal is a direction not a point.
    output.normal               = mul(float4(input.normal, 0.f), LocalToWorldNormal).xyz;

    output.pixel_world_position = mul(input.position, LocalToWorld);
    return output;
}
