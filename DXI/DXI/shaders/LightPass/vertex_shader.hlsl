
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#include "../Common.hlsl"

cbuffer ConstantBufferPerObject : register(b0)
{
    float4x4 LocalToWorld;  // Only with per-object translation
    float4x4 WorldToClip;   // Translation * View * Proj (Clip)
};

// cbuffer ConstantBufferPerCamera: register(b1)
// {
//     //
//     // float4x4 WorldToView;   // World * View
//     float4x4 ViewToClip;    // View * Clip

//     //
//     //float4x4 WorldToClip;   // World * View * Clip
// };

PSInput main(VSInput input)
{
    PSInput output;
    //if input.position is not surrounded by float4, then WorldViewProjectionMat4x4 is truncated to float3x4...
    output.position             = mul(input.position, WorldToClip); 
#if USE_TEXCOORD
    output.tex_coord            = input.tex_coord;
#else
    output.color                = input.color;
#endif
    //output.normal       = input.normal;
    // output.normal       = normalise(float4(input.normal, 1.f), LocalToWorld).xyz; 
    float3 world_normal         = mul(float4(input.normal, 1.f), LocalToWorld).xyz;
    //output.normal               = normalize(world_normal);
    output.normal               = world_normal;

    output.pixel_world_position = mul(input.position, LocalToWorld); 
    return output;
}