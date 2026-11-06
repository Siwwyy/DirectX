
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// What a point light's cube face stores.
//
// Guarded by the same BASELINE as the two vertex shaders. This is the half that decides
// what the atlas holds, so GetShadowFactorCube in Lighting.hlsl has to be read alongside
// it: the two only work together if they agree on the encoding.
#ifdef BASELINE

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

#else // BASELINE

// Your cube face encoding. LightPosition and LightRange arrive as root constants, so the
// result can be normalised against the range as above or left in world units if the
// matching lookup is written to expect that.
float main(ShadowDistancePSInput input) : SV_TARGET
{
    // TODO | Write your own encoding
    //
    // 1.0 is the furthest value the baseline encoding can hold, so a face left like this
    // occludes nothing rather than occluding everything
    return 1.0;
}

#endif // BASELINE
