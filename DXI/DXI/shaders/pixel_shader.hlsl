
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Common.hlsl"
#include "Lighting.hlsl"

// TODO | Rethink those ifdefs...
#if USE_uv
Texture2D t0    : register(t0);
SamplerState s1 : register(s0);
#endif

cbuffer ConstantBufferPerCamera : register(b1)
{
    float4 EyePosition;     // World space position of the camera, Camera::GetPosition()
};

float4 main(PSInput input) : SV_TARGET
{
#if USE_uv
    // Forward path. The shadow atlas is not bound here, and ShadeSurface needs it, so this
    // path is no longer wired up. The deferred LightPass is what runs.
    float3 albedo       = t0.Sample(s1, input.uv).xyz;
    return float4(albedo, 1.0);
#else
    return input.color;
#endif
}
