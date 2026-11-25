//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"
#include "../OverlayLines.hlsl"

// LocalToWorld, WorldToClip and NormalToWorld come from PrimitiveConstantBuffer, and
// WorldToCameraClip, VectorLength, VertexFraction, VectorMask, LineThickness, ViewportSize and
// ShowAllVertices from TangentFrameConstants,
// both in SharedTypes.h

// The convention every tangent space tool follows, so the colours need no legend:
// red along U, green along V, blue out of the surface
#define TANGENT_COLOR           float3(1.00, 0.15, 0.15)
#define BITANGENT_COLOR         float3(0.15, 1.00, 0.15)
#define NORMAL_COLOR            float3(0.25, 0.45, 1.00)

// How bright the end sitting on the vertex is against the tip. The line interpolates between
// the two, so it fades towards the vertex and the direction reads without an arrow head.
#define ROOT_BRIGHTNESS         0.35

// 2^32 divided by the golden ratio, which is the fraction 0.618... in 32 bit fixed point.
// Multiplying by it wraps around modulo 2^32, and that wrap is the fractional part: the
// whole thing is frac(VertexIdx * 0.618...), done exactly in integers.
#define GOLDEN_RATIO_FIXED      2654435769u

// Whether this vertex is one of the Fraction of them that draw.
//
// A hash picked them at random before, and on a mesh of a handful of vertices random can mean
// none at all: 10% of a cube's 8 vertices is a coin flip each, and all of them can lose. This
// is the golden ratio sequence instead, which spreads its values as evenly as any sequence
// can, so every stretch of vertices holds its share of picks whatever the mesh size:
//
//   * Vertex 0 lands on exactly 0, below every fraction above 0, so something always draws.
//   * The count is within a vertex or two of Fraction times the mesh size, for any size.
//   * Fixed per vertex, so the set holds still between frames, and it only grows with the
//     fraction: a vertex shown at 10% is still shown at 20%.
//   * No stripes. Consecutive indices jump by 0.618 around the circle, so neighbours in the
//     mesh's own vertex order are never picked together the way every Nth would pick them.
bool IsVertexPicked(uint VertexIdx, float Fraction)
{
    // The top 24 bits are all a float mantissa holds, so this is exact and never reaches 1
    const uint  Wrapped     = VertexIdx * GOLDEN_RATIO_FIXED;
    const float Position01  = float(Wrapped >> 8u) / 16777216.0;
    return Position01 < Fraction;
}

TangentFramePSInput main(TangentFrameVSInput Input)
{
    TangentFramePSInput Output;

    // Eighteen vertex ids make three quads: which line it is, and which corner of its quad
    const uint LineIdx      = Input.VertexId / TANGENT_FRAME_VERTICES_PER_LINE;     // 0 tangent, 1 bitangent, 2 normal
    const uint CornerIdx    = Input.VertexId % TANGENT_FRAME_VERTICES_PER_LINE;
    const uint TipIdx       = OverlayCornerEnd(CornerIdx);  // 0 on the vertex, 1 at the end of the vector

    // Not every line is wanted. Turned off in the panel, or a vertex outside the share picked.
    // Every corner is sent to the same point outside the clip volume, which leaves degenerate
    // triangles that are dropped before they are rasterized, so a skipped line costs no pixels.
    //
    // The pick is made for the vertex, never for a line: a picked vertex draws every vector
    // switched on, so its tangent, bitangent and normal always show up together. A mesh the CPU
    // flagged, a small one or the built in cube and plane, skips the pick and draws all of its
    // vertices.
    const bool bVertexShown = ShowAllVertices != 0u
                            || IsVertexPicked(Input.InstanceId, VertexFraction);
    const bool bShown       = ((VectorMask >> LineIdx) & 1u) != 0u && bVertexShown;

    if (!bShown)
    {
        Output.Position = OVERLAY_HIDDEN_POSITION;
        Output.Color    = float4(0.0, 0.0, 0.0, 0.0);
        return Output;
    }

    // The three vectors in world space, each through the matrix it needs. The tangent and
    // bitangent lie along the surface and move with it, the normal has to stay
    // perpendicular to it, see the base pass vertex shader for why.
    const float3 WorldTangent   = mul(float4(Input.Tangent.xyz,   0.0), LocalToWorld).xyz;
    const float3 WorldBitangent = mul(float4(Input.Bitangent.xyz, 0.0), LocalToWorld).xyz;
    const float3 WorldNormal    = mul(float4(Input.Normal,        0.0), NormalToWorld).xyz;

    float3 Direction    = LineIdx == 0u ? WorldTangent : (LineIdx == 1u ? WorldBitangent : WorldNormal);
    float3 Color        = LineIdx == 0u ? TANGENT_COLOR : (LineIdx == 1u ? BITANGENT_COLOR : NORMAL_COLOR);

    // The unit vector, drawn VectorLength long. A scaled object stretches the vectors the
    // matrices carry, which is not what is being inspected, so the length is normalized away.
    Direction = SafeNormalize(Direction);

    // The root goes through exactly the matrix the base pass used for this vertex, so it
    // lands on the surface to the last bit and the depth test has nothing to fight with.
    // The tip is the root pushed by the vector, which is linear in clip space, so it is the
    // same offset put through the camera matrix on its own.
    const float4 RootClip   = mul(Input.Position, WorldToClip);
    const float4 TipOffset  = mul(float4(Direction * VectorLength, 0.0), WorldToCameraClip);

    // Both ends of the line, since the width is worked out across it and every corner needs
    // to know where the line is going, not just where its own end is. A line behind the camera
    // is trimmed or dropped by the shared helper, and a root in front stays exactly where the
    // base pass put the surface.
    const OverlayLine Line = BuildOverlayLine(RootClip, RootClip + TipOffset, ViewportSize);

    if (!Line.bVisible)
    {
        Output.Position = OVERLAY_HIDDEN_POSITION;
        Output.Color    = float4(0.0, 0.0, 0.0, 0.0);
        return Output;
    }

    // This corner of the quad, moved sideways by half the width. z and w are the end's own, so
    // the depth along the line is the same as before and the bias against the surface holds.
    Output.Position = PlaceOverlayLineCorner(Line, CornerIdx, LineThickness, ViewportSize);

    const float Brightness  = lerp(ROOT_BRIGHTNESS, 1.0, float(TipIdx));
    Output.Color            = float4(Color * Brightness, 1.0);

    return Output;
}
