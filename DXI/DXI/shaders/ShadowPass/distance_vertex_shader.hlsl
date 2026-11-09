
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// One face of a point light's cube. The world position is carried through, because the
// pixel shader stores how far it is from the light rather than a depth.
ShadowDistancePSInput main(VSInput Input)
{
    ShadowDistancePSInput Output;

    float4 World                = mul(Input.Position, LocalToWorld);

    Output.Position             = mul(World, WorldToLightClip);
    Output.WorldPosition       = World.xyz;

    return Output;
}
