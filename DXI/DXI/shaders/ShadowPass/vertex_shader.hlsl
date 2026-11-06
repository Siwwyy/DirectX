
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Directional and area lights, depth only. There is no pixel shader attached to this
// pipeline, the rasterizer fills the shadow slice straight out of SV_POSITION.
//
// BASELINE is defined by ShadowPass.cpp alongside SHADOW_PASS. Turn it off there to write
// your own, and see the note in Lighting.hlsl part 2: what this pass writes and what the
// lookup expects have to agree, so changing one normally means changing both.
#ifdef BASELINE

float4 main(VSInput input) : SV_POSITION
{
    // Two steps instead of one baked matrix, because the same object is rendered from
    // several lights in a frame and only the second half changes between them
    float4 world = mul(input.position, LocalToWorld);
    return mul(world, WorldToLightClip);
}

#else // BASELINE

// Your depth rendering. LocalToWorld and WorldToLightClip are both in the per object
// constant buffer, the second one supplied as root constants per light per frame.
float4 main(VSInput input) : SV_POSITION
{
    // TODO | Write your own shadow depth transform
    //
    // Clipped rather than collapsed: z beyond w puts this past the far plane, so the
    // slice is left as the pass cleared it instead of being stamped with a degenerate
    // triangle at the near plane
    return float4(0.0, 0.0, 2.0, 1.0);
}

#endif // BASELINE
