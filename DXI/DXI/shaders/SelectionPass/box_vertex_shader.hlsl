//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"
#include "../OverlayLines.hlsl"

// WorldToClip comes from PrimitiveConstantBuffer, and BoxMin, BoxMax, SelectionLineThickness,
// SelectionColor and SelectionViewport from SelectionConstants, both in SharedTypes.h

// Six corners make the quad of one edge, see OverlayLines.hlsl
#define BOX_VERTICES_PER_EDGE   6u

// Edges that run along each axis, four of them
#define BOX_EDGES_PER_AXIS      4u

SelectionPSInput main(SelectionBoxVSInput Input)
{
    SelectionPSInput Output;

    // Seventy two vertex ids make twelve quads: which edge it is, and which corner of its quad
    const uint EdgeIdx      = Input.VertexId / BOX_VERTICES_PER_EDGE;
    const uint CornerIdx    = Input.VertexId % BOX_VERTICES_PER_EDGE;

    // Four edges run along each of the three axes. The edge's two ends differ only along its own
    // axis, and which of the four it is picks where it sits on the other two.
    const uint Axis         = EdgeIdx / BOX_EDGES_PER_AXIS;
    const uint Sibling      = EdgeIdx % BOX_EDGES_PER_AXIS;
    const uint AxisA        = (Axis + 1u) % 3u;
    const uint AxisB        = (Axis + 2u) % 3u;

    // A corner of the box as three bits, one per axis, set meaning the large end of that axis.
    // The root is at the small end of its own axis and the tip at the large end.
    const uint RootBits     = ((Sibling & 1u) << AxisA) | (((Sibling >> 1u) & 1u) << AxisB);
    const uint TipBits      = RootBits | (1u << Axis);

    const float3 RootLocal  = lerp(BoxMin, BoxMax, float3(float(RootBits & 1u), float((RootBits >> 1u) & 1u), float((RootBits >> 2u) & 1u)));
    const float3 TipLocal   = lerp(BoxMin, BoxMax, float3(float(TipBits & 1u),  float((TipBits >> 1u) & 1u),  float((TipBits >> 2u) & 1u)));

    // Both ends go through the object's own matrices like a vertex of the mesh would, which
    // turns and scales the box with the object
    const float4 RootClip   = mul(float4(RootLocal, 1.0), WorldToClip);
    const float4 TipClip    = mul(float4(TipLocal,  1.0), WorldToClip);

    // Behind the camera it is trimmed or dropped, the same as the other overlays' lines
    const OverlayLine Line  = BuildOverlayLine(RootClip, TipClip, SelectionViewport);

    if (!Line.bVisible)
    {
        Output.Position = OVERLAY_HIDDEN_POSITION;
        Output.Color    = float4(0.0, 0.0, 0.0, 0.0);
        return Output;
    }

    Output.Position = PlaceOverlayLineCorner(Line, CornerIdx, SelectionLineThickness, SelectionViewport);

    // Solid, whatever the tint's strength is
    Output.Color    = float4(SelectionColor.rgb, 1.0);

    return Output;
}
