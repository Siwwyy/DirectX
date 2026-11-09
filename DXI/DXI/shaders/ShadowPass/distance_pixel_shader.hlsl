
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// What a point light's cube face stores. This is the half that decides what the atlas
// holds, so GetShadowFactorCube in Lighting.hlsl has to be read alongside it: the two only
// work together if they agree on the encoding.
//
// Distance from the light, divided by its range so it fits the 0 to 1 the target holds.
//
// Storing distance rather than depth is what makes the lookup cheap later: a point light
// is sampled with a direction, and the value that comes back can be compared against the
// distance to the shaded pixel directly, with no projection and no face maths.
float main(ShadowDistancePSInput Input) : SV_TARGET
{
    float DistanceToLight = length(Input.WorldPosition - LightPosition);

    return saturate(DistanceToLight / max(LightRange, 1e-4));
}
