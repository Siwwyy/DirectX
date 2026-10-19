
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Depth only. There is no pixel shader attached to this pipeline, the rasterizer fills
// the shadow slice straight out of SV_POSITION.
float4 main(VSInput input) : SV_POSITION
{
    // Two steps instead of one baked matrix, because the same object is rendered from
    // several lights in a frame and only the second half changes between them
    float4 world = mul(input.position, LocalToWorld);
    return mul(world, WorldToLightClip);
}
