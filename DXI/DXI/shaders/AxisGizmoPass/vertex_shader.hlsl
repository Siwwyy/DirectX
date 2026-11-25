//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"
#include "../OverlayLines.hlsl"

// LocalToWorld and WorldToClip come from PrimitiveConstantBuffer, and WorldToCameraClip,
// AxisLength, LineThickness, HeadSize and ViewportSize from AxisGizmoConstants, both in
// SharedTypes.h

// The convention of every 3D tool, so the colours need no legend: X red, Y green, Z blue
#define AXIS_X_COLOR            float3(1.00, 0.20, 0.20)
#define AXIS_Y_COLOR            float3(0.20, 1.00, 0.20)
#define AXIS_Z_COLOR            float3(0.25, 0.45, 1.00)

AxisGizmoPSInput main(AxisGizmoVSInput Input)
{
    AxisGizmoPSInput Output;

    // Twenty seven vertex ids make three arrows: which axis it is, and which part of the arrow
    const uint AxisIdx  = Input.VertexId / AXIS_GIZMO_VERTICES_PER_ARROW;   // 0 X, 1 Y, 2 Z
    const uint PartIdx  = Input.VertexId % AXIS_GIZMO_VERTICES_PER_ARROW;   // 0 to 5 the shaft, 6 to 8 the head

    const float3 LocalAxis  = AxisIdx == 0u ? float3(1.0, 0.0, 0.0) : (AxisIdx == 1u ? float3(0.0, 1.0, 0.0) : float3(0.0, 0.0, 1.0));
    const float3 Color      = AxisIdx == 0u ? AXIS_X_COLOR : (AxisIdx == 1u ? AXIS_Y_COLOR : AXIS_Z_COLOR);

    // The object's own axis in the world. The object's rotation carries it, and the scale
    // is normalized away: the arrow is as long as the panel says, whatever size the object is.
    const float3 WorldAxis = SafeNormalize(mul(float4(LocalAxis, 0.0), LocalToWorld).xyz);

    // The origin is the object's position, where a local zero lands. It goes through the object's
    // own matrix like any vertex would. The tip is the origin pushed along the axis, which is
    // linear in clip space, so it is the same offset put through the camera matrix on its own.
    const float4 OriginClip = mul(float4(0.0, 0.0, 0.0, 1.0), WorldToClip);
    const float4 TipOffset  = mul(float4(WorldAxis * AxisLength, 0.0), WorldToCameraClip);

    // Behind the camera it is trimmed or dropped, the same as the tangent frame's lines
    const OverlayLine Line = BuildOverlayLine(OriginClip, OriginClip + TipOffset, ViewportSize);

    // The head sits on the tip, so a tip that was trimmed has no head: it would show up
    // where the arrow meets the camera plane, as if the arrow ended there
    const bool bIsHead  = PartIdx >= AXIS_GIZMO_SHAFT_CORNERS;
    if (!Line.bVisible || (bIsHead && Line.bTipTrimmed))
    {
        Output.Position = OVERLAY_HIDDEN_POSITION;
        Output.Color    = float4(0.0, 0.0, 0.0, 0.0);
        return Output;
    }

    Output.Position = bIsHead
        ? PlaceOverlayArrowHeadCorner(Line, PartIdx - AXIS_GIZMO_SHAFT_CORNERS, HeadSize, ViewportSize)
        : PlaceOverlayLineCorner(Line, PartIdx, LineThickness, ViewportSize);
    Output.Color    = float4(Color, 1.0);

    return Output;
}
