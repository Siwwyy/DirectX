
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Common.hlsl"
#include "Lighting.hlsl"

// TODO | Rethink those ifdefs...
#if USE_TEXCOORD
Texture2D t0    : register(t0);
SamplerState s1 : register(s0);
#endif

cbuffer ConstantBufferPerCamera : register(b1)
{
    float4 EyePosition;     // World space position of the camera, Camera::GetPosition()
};

float4 main(PSInput input) : SV_TARGET
{
#if USE_TEXCOORD
    // Forward path, there is no shadow map bound here so nothing is occluded
    float3 albedo       = t0.Sample(s1, input.tex_coord).xyz;
    float3 output_color = ShadeSurface(albedo, input.normal, input.pixel_world_position.xyz, EyePosition.xyz, 1.0);
    return float4(output_color, 1.0);
#else
    return input.color;
#endif
}
