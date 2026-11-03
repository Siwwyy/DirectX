
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Distance from the light, divided by its range so it fits the 0 to 1 the target holds.
//
// Storing distance rather than depth is what makes the lookup cheap later: a point light
// is sampled with a direction, and the value that comes back can be compared against the
// distance to the shaded pixel directly, with no projection and no face maths.
float main(ShadowDistancePSInput input) : SV_TARGET
{
    float DistanceToLight = length(input.world_position - LightPosition);

    return saturate(DistanceToLight / max(LightRange, 1e-4));
}
