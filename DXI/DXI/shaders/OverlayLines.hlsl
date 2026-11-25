//Copyright, Damian Andrysiak 2026, All Rights Reserved.

// Lines with a width, for the debug overlays. Hardware lines are always one pixel wide, so a
// line here is a quad of two triangles whose corners the vertex shader pushes sideways on
// screen. Shared by the tangent frame and the origin axes, which differ in what they draw
// and not in how a line becomes pixels.
//
// Nothing in here reads a constant buffer: everything comes in as a parameter, so a shader
// can include it whatever it binds.

#ifndef OVERLAY_LINES_HLSL_INCLUDED
#define OVERLAY_LINES_HLSL_INCLUDED

// Where a hidden vertex goes. Outside the clip volume, and every corner of a hidden quad goes
// to the same place, which leaves degenerate triangles that are dropped before they are
// rasterized, so something hidden costs no pixels.
#define OVERLAY_HIDDEN_POSITION float4(2.0, 2.0, 2.0, 1.0)

// Closest a clip space end may sit to the camera plane before the line is shortened to it.
// An end at or behind it has no screen position to expand the quad from.
#define OVERLAY_MIN_CLIP_W      1e-4

// Unit length, or nothing at all for a vector that has none. A vector that collapsed onto
// zero has no direction to draw, and normalizing it would put NaN on screen.
float3 SafeNormalize(float3 Vector)
{
    float LengthSqr = dot(Vector, Vector);
    return LengthSqr > 1e-12 ? Vector * rsqrt(LengthSqr) : float3(0.0, 0.0, 0.0);
}

// The six corners of a line's quad as two triangles. Which end of the line each one sits at,
// and which side of it: the first triangle is root-, root+, tip-, the second tip-, root+, tip+.
static const uint  OVERLAY_CORNER_END[6]  = { 0u, 0u, 1u, 1u, 0u, 1u };
static const float OVERLAY_CORNER_SIDE[6] = { -1.0, 1.0, -1.0, -1.0, 1.0, 1.0 };

// A line ready to have its corners placed: both ends in clip space after being kept in front
// of the camera, and the direction it runs in on screen.
struct OverlayLine
{
    float4 RootEnd;         // Clip space, never behind the camera plane
    float4 TipEnd;          // Same
    float2 PixelDirection;  // Unit, on screen, from the root towards the tip
    float  PixelLength;     // How long it is on screen, after trimming
    bool   bVisible;        // False when the whole line is behind the camera
    bool   bTipTrimmed;     // True when the tip was moved, so it is not where the vector ends
};

// Which end of the line a corner sits at, 0 for the root and 1 for the tip
uint OverlayCornerEnd(uint CornerIdx)
{
    return OVERLAY_CORNER_END[CornerIdx];
}

/**
 * Takes the two ends of a line in clip space and gets it ready to draw.
 *
 * A line that runs behind the camera has no screen position there, and dividing by a w that
 * has gone negative flips it across the screen. Both ends behind it, nothing to draw. One end
 * behind it, the line is shortened to where it meets the camera plane, which is all of it the
 * hardware would have clipped to anyway. Ends in front are left untouched, so a root that sits
 * exactly on a surface stays exactly there.
 */
OverlayLine BuildOverlayLine(float4 RootClip, float4 TipClip, float2 ViewportSize)
{
    OverlayLine Line;
    Line.RootEnd        = RootClip;
    Line.TipEnd         = TipClip;
    Line.PixelDirection = float2(1.0, 0.0);
    Line.PixelLength    = 0.0;
    Line.bVisible       = true;
    Line.bTipTrimmed    = false;

    if (RootClip.w < OVERLAY_MIN_CLIP_W && TipClip.w < OVERLAY_MIN_CLIP_W)
    {
        Line.bVisible = false;
        return Line;
    }

    if (RootClip.w < OVERLAY_MIN_CLIP_W)
    {
        Line.RootEnd = lerp(RootClip, TipClip, (OVERLAY_MIN_CLIP_W - RootClip.w) / (TipClip.w - RootClip.w));
    }
    else if (TipClip.w < OVERLAY_MIN_CLIP_W)
    {
        Line.TipEnd         = lerp(RootClip, TipClip, (OVERLAY_MIN_CLIP_W - RootClip.w) / (TipClip.w - RootClip.w));
        Line.bTipTrimmed    = true;
    }

    // To pixels, where a width means something. NDC runs -1 to 1 across the target, so it is
    // half the viewport in pixels per unit.
    const float2 HalfViewport   = ViewportSize * 0.5;
    const float2 RootPixel      = (Line.RootEnd.xy / Line.RootEnd.w) * HalfViewport;
    const float2 TipPixel       = (Line.TipEnd.xy / Line.TipEnd.w) * HalfViewport;

    // A line seen end on is a point, with no direction to stretch across. Any would do, the
    // quad is only a few pixels either way.
    const float2 PixelDelta     = TipPixel - RootPixel;
    const float  LengthSqr      = dot(PixelDelta, PixelDelta);
    Line.PixelLength            = sqrt(LengthSqr);
    Line.PixelDirection         = LengthSqr > 1e-8 ? PixelDelta * rsqrt(LengthSqr) : float2(1.0, 0.0);

    return Line;
}

/**
 * Clip space position of one corner of the line's quad.
 *
 * Perpendicular to the line on screen, half the width out to either side of it. Back through
 * the half viewport to land in NDC, then through w to be clip space again. z and w stay the
 * end's own, so the depth along the line is the same as before and a depth bias still holds.
 */
float4 PlaceOverlayLineCorner(OverlayLine Line, uint CornerIdx, float ThicknessPixels, float2 ViewportSize)
{
    const float2 HalfViewport   = ViewportSize * 0.5;
    const float  Side           = OVERLAY_CORNER_SIDE[CornerIdx];
    const float2 PixelNormal    = float2(-Line.PixelDirection.y, Line.PixelDirection.x);
    const float2 NdcOffset      = (PixelNormal * (Side * ThicknessPixels * 0.5)) / HalfViewport;

    const float4 CornerEnd      = OVERLAY_CORNER_END[CornerIdx] == 0u ? Line.RootEnd : Line.TipEnd;
    return float4(CornerEnd.xy + (NdcOffset * CornerEnd.w), CornerEnd.z, CornerEnd.w);
}

/**
 * Clip space position of one corner of an arrow head: a triangle on the tip of the line,
 * its point on the tip and its base HeadPixels back along the line, as wide as it is long
 * across. Sized in pixels, so it reads the same at any distance. Shrinks to the line when
 * the line is shorter on screen than the head would be, instead of reaching past the root.
 *
 * Corner 0 is the point, 1 and 2 the two ends of the base.
 */
float4 PlaceOverlayArrowHeadCorner(OverlayLine Line, uint HeadCornerIdx, float HeadPixels, float2 ViewportSize)
{
    const float2 HalfViewport   = ViewportSize * 0.5;
    const float  Head           = min(HeadPixels, Line.PixelLength);

    // Corner 0 stays where it is. The other two go back along the line and out to either side.
    const float  Back           = HeadCornerIdx == 0u ? 0.0 : Head;
    const float  Out            = HeadCornerIdx == 0u ? 0.0 : (HeadCornerIdx == 1u ? 0.5 : -0.5) * Head;

    const float2 PixelNormal    = float2(-Line.PixelDirection.y, Line.PixelDirection.x);
    const float2 NdcOffset      = ((PixelNormal * Out) - (Line.PixelDirection * Back)) / HalfViewport;

    return float4(Line.TipEnd.xy + (NdcOffset * Line.TipEnd.w), Line.TipEnd.z, Line.TipEnd.w);
}

#endif // OVERLAY_LINES_HLSL_INCLUDED
