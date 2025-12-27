
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Common.hlsl"

// TODO | Rethink those ifdefs...
#if USE_TEXCOORD
Texture2D t0    : register(t0);
SamplerState s1 : register(s0);
#endif

float4 main(PSInput input) : SV_TARGET
{
#if USE_TEXCOORD
    return t0.Sample(s1, input.tex_coord);
#else
    return input.color;
#endif
}