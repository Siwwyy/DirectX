
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Directional and area lights, depth only. There is no pixel shader attached to this
// pipeline, the rasterizer fills the shadow slice straight out of SV_POSITION.
//
// What this pass writes and what GetShadowFactor2D in Lighting.hlsl expects have to agree,
// so changing one normally means changing both.
float4 main(VSInput Input) : SV_POSITION
{
    // Two steps instead of one baked matrix, because the same object is rendered from
    // several lights in a frame and only the second half changes between them
    float4 World = mul(Input.Position, LocalToWorld);
    return mul(World, WorldToLightClip);
}
