
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

cbuffer ConstantBufferPerObject : register(b0)
{
    float4x4 LocalToWorld;          // Only with per-object translation
    float4x4 WorldToClip;           // Translation * View * Proj (Clip) | the camera, unused here
    float4x4 LocalToLightClip;      // Translation * Light View * Light Proj
    float4x4 LocalToWorldNormal;    // Inverse transpose of LocalToWorld | unused here, depth only
};

// Depth only. There is no pixel shader attached to this pass, the rasterizer fills
// the shadow map straight out of SV_POSITION.
float4 main(VSInput input) : SV_POSITION
{
    return mul(input.position, LocalToLightClip);
}
