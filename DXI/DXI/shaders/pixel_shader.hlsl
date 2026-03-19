
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Common.hlsl"

// TODO | Rethink those ifdefs...
#if USE_TEXCOORD
Texture2D t0    : register(t0);
SamplerState s1 : register(s0);
#endif

float4 main(PSInput input) : SV_TARGET
{
    float3 light_color = float3(1,1,0);
    float ambient_strength = 0.7;
    float3 ambient = ambient_strength * light_color;

#if USE_TEXCOORD
    return float4(ambient, 1.0) * t0.Sample(s1, input.tex_coord);
#else
    return input.color;
#endif
}