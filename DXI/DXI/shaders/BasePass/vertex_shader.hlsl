
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "../Common.hlsl"

// LocalToWorld, WorldToClip and LocalToWorldNormal come from PrimitiveConstantBuffer in
// SharedTypes.h, the one definition the C++ upload path writes into

PSInput main(VSInput input)
{
    PSInput output;

    //if input.position is not surrounded by float4, then WorldViewProjectionMat4x4 is truncated to float3x4...
    output.position             = mul(input.position, WorldToClip);
    output.uv            = input.uv;

    // The normal gets its own matrix, LocalToWorld would shear it under a non uniform
    // scale. w of 0 keeps the translation out of it, a normal is a direction not a point.
    output.normal               = mul(float4(input.normal, 0.f), LocalToWorldNormal).xyz;

    // The tangent lies along the surface, so it transforms like a position direction
    // rather than like a normal. Handedness is carried through untouched.
    output.tangent              = float4(mul(float4(input.tangent.xyz, 0.f), LocalToWorld).xyz, input.tangent.w);

    // Same for the bitangent, it is the other axis lying in the surface
    output.bitangent            = float4(mul(float4(input.bitangent.xyz, 0.f), LocalToWorld).xyz, 0.f);

    output.world_position = mul(input.position, LocalToWorld);
    return output;
}
