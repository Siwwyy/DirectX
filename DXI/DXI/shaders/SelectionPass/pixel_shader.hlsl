//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Nothing to shade, the vertex shader already decided the colour. Serves both the tint and the box.
float4 main(SelectionPSInput Input) : SV_TARGET
{
    return Input.Color;
}
