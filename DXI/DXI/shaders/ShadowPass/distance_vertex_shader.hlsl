
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// One face of a point light's cube. The world position is carried through, because the
// pixel shader stores how far it is from the light rather than a depth.
ShadowDistancePSInput main(VSInput input)
{
    ShadowDistancePSInput output;

    float4 world                = mul(input.position, LocalToWorld);

    output.position             = mul(world, WorldToLightClip);
    output.world_position       = world.xyz;

    return output;
}
