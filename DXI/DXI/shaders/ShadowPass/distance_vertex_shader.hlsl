
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// One face of a point light's cube. The world position is carried through, because the
// pixel shader stores how far it is from the light rather than a depth.
//
// Guarded by the same BASELINE as the depth only path, see vertex_shader.hlsl.
#ifdef BASELINE

ShadowDistancePSInput main(VSInput input)
{
    ShadowDistancePSInput output;

    float4 world                = mul(input.position, LocalToWorld);

    output.position             = mul(world, WorldToLightClip);
    output.world_position       = world.xyz;

    return output;
}

#else // BASELINE

// Your cube face transform. Whatever the pixel shader needs has to be carried through
// ShadowDistancePSInput, which is declared in Common.hlsl and can be extended.
ShadowDistancePSInput main(VSInput input)
{
    ShadowDistancePSInput output;

    // TODO | Write your own cube face transform. Clipped for now, as in vertex_shader.hlsl
    output.position             = float4(0.0, 0.0, 2.0, 1.0);
    output.world_position       = float3(0.0, 0.0, 0.0);

    return output;
}

#endif // BASELINE
