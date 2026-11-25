//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"
#include "../OverlayLines.hlsl"

// WorldToCameraClip, GizmoOrigin, GizmoLength, GizmoAxisX/Y/Z, GizmoMode, GizmoHoveredAxis,
// GizmoActiveAxis, GizmoCameraPosition, GizmoLineThickness, GizmoViewport and GizmoHeadSize come
// from TransformGizmoConstants in SharedTypes.h

// The convention of every 3D tool, so the colours need no legend: X red, Y green, Z blue
#define GIZMO_X_COLOR               float3(1.00, 0.20, 0.20)
#define GIZMO_Y_COLOR               float3(0.20, 1.00, 0.20)
#define GIZMO_Z_COLOR               float3(0.25, 0.45, 1.00)

// What the handle under the cursor, or in the hand, turns into
#define GIZMO_HIGHLIGHT_COLOR       float3(1.00, 0.90, 0.25)

// The half of a ring behind the object is drawn darker, which is what makes a flat circle read as a ring in space
#define GIZMO_FAR_BRIGHTNESS        0.40

// A held handle is drawn this much thicker, so it is plain which one is being used
#define GIZMO_ACTIVE_THICKNESS      1.5

// The line a ring turns about is thinner than the ring and reaches a little beyond it
#define GIZMO_AXIS_LINE_THICKNESS   0.5
#define GIZMO_AXIS_LINE_OVERSHOOT   1.15

// A full turn in radians
#define GIZMO_TWO_PI                6.28318530718

float3 GetGizmoAxis(uint AxisIdx)
{
    return AxisIdx == 0u ? GizmoAxisX : (AxisIdx == 1u ? GizmoAxisY : GizmoAxisZ);
}

float3 GetGizmoColor(uint AxisIdx)
{
    return AxisIdx == 0u ? GIZMO_X_COLOR : (AxisIdx == 1u ? GIZMO_Y_COLOR : GIZMO_Z_COLOR);
}

// A direction offset from the centre as a clip space offset. Linear, so it is the offset put
// through the camera matrix on its own and added to where the centre lands.
float4 GetClipOffset(float3 WorldOffset)
{
    return mul(float4(WorldOffset, 0.0), WorldToCameraClip);
}

TransformGizmoPSInput GetHidden()
{
    TransformGizmoPSInput Hidden;
    Hidden.Position = OVERLAY_HIDDEN_POSITION;
    Hidden.Color    = float4(0.0, 0.0, 0.0, 0.0);
    return Hidden;
}

TransformGizmoPSInput main(TransformGizmoVSInput Input)
{
    TransformGizmoPSInput Output;

    const float4 OriginClip = mul(float4(GizmoOrigin, 1.0), WorldToCameraClip);

    uint    AxisIdx         = 0u;
    uint    CornerIdx       = 0u;       // Which corner of the quad this is, 0 to 5, or of the head, 0 to 2
    bool    bIsHead         = false;
    float   Thickness       = GizmoLineThickness;
    float   Brightness      = 1.0;
    float4  RootClip        = OriginClip;
    float4  TipClip         = OriginClip;

    if (GizmoMode == TRANSFORM_GIZMO_MODE_MOVE)
    {
        // Twenty seven vertex ids make three arrows: which axis it is, and which part of the arrow
        AxisIdx     = Input.VertexId / TRANSFORM_GIZMO_VERTICES_PER_ARROW;
        const uint PartIdx = Input.VertexId % TRANSFORM_GIZMO_VERTICES_PER_ARROW;

        // 0 to 5 are the shaft, 6 to 8 the head
        bIsHead     = PartIdx >= 6u;
        CornerIdx   = bIsHead ? (PartIdx - 6u) : PartIdx;

        TipClip     = OriginClip + GetClipOffset(GetGizmoAxis(AxisIdx) * GizmoLength);
    }
    else if (Input.VertexId < TRANSFORM_GIZMO_RING_VERTEX_COUNT)
    {
        // A ring: which axis it turns about, which of its sixty four short lines this is, and the corner of that line's quad
        AxisIdx                     = Input.VertexId / TRANSFORM_GIZMO_VERTICES_PER_RING;
        const uint WithinRing       = Input.VertexId % TRANSFORM_GIZMO_VERTICES_PER_RING;
        const uint SegmentIdx       = WithinRing / TRANSFORM_GIZMO_VERTICES_PER_LINE;
        CornerIdx                   = WithinRing % TRANSFORM_GIZMO_VERTICES_PER_LINE;

        // The ring lies in the plane the axis is the normal of, spanned by the other two. Which of
        // them is first and which second does not matter for drawing, only for picking, and the
        // picking in TransformGizmo.cpp takes them in this same order.
        const float3 U              = GetGizmoAxis((AxisIdx + 1u) % 3u);
        const float3 V              = GetGizmoAxis((AxisIdx + 2u) % 3u);

        const float StartAngle      = (float(SegmentIdx)      / float(TRANSFORM_GIZMO_RING_SEGMENTS)) * GIZMO_TWO_PI;
        const float EndAngle        = (float(SegmentIdx + 1u) / float(TRANSFORM_GIZMO_RING_SEGMENTS)) * GIZMO_TWO_PI;

        const float3 StartOffset    = ((U * cos(StartAngle)) + (V * sin(StartAngle))) * GizmoLength;
        const float3 EndOffset      = ((U * cos(EndAngle))   + (V * sin(EndAngle)))   * GizmoLength;

        RootClip                    = OriginClip + GetClipOffset(StartOffset);
        TipClip                     = OriginClip + GetClipOffset(EndOffset);

        // The half that is on the far side of the centre from the camera
        const float3 ToCamera       = GizmoCameraPosition - GizmoOrigin;
        Brightness                  = dot((StartOffset + EndOffset), ToCamera) < 0.0 ? GIZMO_FAR_BRIGHTNESS : 1.0;
    }
    else
    {
        // The line a ring turns about, through the centre and out a little past the ring on both sides
        const uint LineVertexIdx    = Input.VertexId - TRANSFORM_GIZMO_RING_VERTEX_COUNT;
        AxisIdx                     = LineVertexIdx / TRANSFORM_GIZMO_AXIS_LINE_VERTICES;
        CornerIdx                   = LineVertexIdx % TRANSFORM_GIZMO_AXIS_LINE_VERTICES;
        Thickness                   = GizmoLineThickness * GIZMO_AXIS_LINE_THICKNESS;

        const float3 Reach          = GetGizmoAxis(AxisIdx) * (GizmoLength * GIZMO_AXIS_LINE_OVERSHOOT);
        RootClip                    = OriginClip - GetClipOffset(Reach);
        TipClip                     = OriginClip + GetClipOffset(Reach);
    }

    // Holding a ring leaves only that ring, with its axis, on the screen: the others would
    // cross it and hide what is being turned. Arrows stay, there is nothing to hide there.
    if (GizmoMode == TRANSFORM_GIZMO_MODE_ROTATE
        && GizmoActiveAxis != TRANSFORM_GIZMO_AXIS_NONE
        && AxisIdx != GizmoActiveAxis)
    {
        return GetHidden();
    }

    // Behind the camera it is trimmed or dropped, the same as the other overlays' lines. The
    // head sits on the tip, so a tip that was trimmed has no head.
    const OverlayLine Line = BuildOverlayLine(RootClip, TipClip, GizmoViewport);
    if (!Line.bVisible || (bIsHead && Line.bTipTrimmed))
    {
        return GetHidden();
    }

    // The handle under the cursor or in the hand shows in the highlight colour
    const bool bHovered = AxisIdx == GizmoHoveredAxis;
    const bool bActive  = AxisIdx == GizmoActiveAxis;
    float3 Color        = GetGizmoColor(AxisIdx);
    if (bHovered || bActive)
    {
        Color = GIZMO_HIGHLIGHT_COLOR;
    }
    if (bActive)
    {
        Thickness *= GIZMO_ACTIVE_THICKNESS;
    }

    Output.Position = bIsHead
        ? PlaceOverlayArrowHeadCorner(Line, CornerIdx, GizmoHeadSize, GizmoViewport)
        : PlaceOverlayLineCorner(Line, CornerIdx, Thickness, GizmoViewport);
    Output.Color    = float4(Color * Brightness, 1.0);

    return Output;
}
