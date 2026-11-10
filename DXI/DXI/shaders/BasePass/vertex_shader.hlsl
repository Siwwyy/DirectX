
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "../Common.hlsl"

// LocalToWorld, WorldToClip and LocalToWorldUniformScale come from PrimitiveConstantBuffer in
// SharedTypes.h, the one definition the C++ upload path writes into

PSInput main(VSInput Input)
{
    PSInput Output;

    //if Input.Position is not surrounded by float4, then WorldViewProjectionMat4x4 is truncated to float3x4...
    Output.Position             = mul(Input.Position, WorldToClip);
    Output.UV            = Input.UV;

    // Same vertex through this frame's and the previous frame's matrices. The difference
    // between where the two land on screen is what the velocity target stores.
    Output.CurrentClip          = Output.Position;
    Output.PrevClip             = mul(Input.Position, PrevWorldToClip);

    // The normal gets its own matrix, LocalToWorld would shear it under a non uniform
    // scale. w of 0 keeps the translation out of it, a normal is a direction not a point.
    Output.Normal               = mul(float4(Input.Normal, 0.f), LocalToWorldUniformScale).xyz;

    // The tangent lies along the surface, so it transforms like a position direction
    // rather than like a normal. Handedness is carried through untouched.
    Output.Tangent              = float4(mul(float4(Input.Tangent.xyz, 0.f), LocalToWorld).xyz, Input.Tangent.w);

    // Same for the bitangent, it is the other axis lying in the surface
    Output.Bitangent            = float4(mul(float4(Input.Bitangent.xyz, 0.f), LocalToWorld).xyz, 0.f);

    Output.WorldPosition = mul(Input.Position, LocalToWorld);
    return Output;
}
