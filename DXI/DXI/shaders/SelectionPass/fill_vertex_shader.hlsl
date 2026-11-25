//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// WorldToClip comes from PrimitiveConstantBuffer and SelectionColor from SelectionConstants,
// both in SharedTypes.h

SelectionPSInput main(SelectionFillVSInput Input)
{
    SelectionPSInput Output;

    // Through exactly the matrix the base pass used for this vertex, so it lands on the surface
    // that was just drawn and the depth test has nothing to fight with beyond the bias
    Output.Position = mul(Input.Position, WorldToClip);

    // The alpha is how strongly the surface is tinted, the blend state does the rest
    Output.Color    = SelectionColor;

    return Output;
}
