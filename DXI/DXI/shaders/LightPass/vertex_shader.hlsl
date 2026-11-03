
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "../Common.hlsl"

// Fullscreen triangle generated from the vertex id, so this pass needs no vertex
// or index buffer at all. Draw it with DrawInstanced(3, 1, 0, 0).
//
//   id 0 -> uv (0,0) -> clip (-1,  1)
//   id 1 -> uv (2,0) -> clip ( 3,  1)
//   id 2 -> uv (0,2) -> clip (-1, -3)
//
// One oversized triangle covers the whole viewport and avoids the seam two quad halves would have.
FullscreenPSInput main(uint vertex_id : SV_VertexID)
{
    FullscreenPSInput output;

    output.tex_coord    = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position     = float4(output.tex_coord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return output;
}
