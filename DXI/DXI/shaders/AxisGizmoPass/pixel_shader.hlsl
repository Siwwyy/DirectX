//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Nothing to shade, the vertex shader already decided the colour
float4 main(AxisGizmoPSInput Input) : SV_TARGET
{
    return Input.Color;
}
