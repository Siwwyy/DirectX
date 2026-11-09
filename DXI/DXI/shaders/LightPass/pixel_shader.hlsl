
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "../Common.hlsl"
#include "../Lighting.hlsl"

// G-buffer written by BasePass, registers follow the GBUFFER_* order of SharedTypes.h,
// then depth, the shadow map, and the two sky cubes after them.
//
// Depth is the base pass's own depth target, sampled through an R32_FLOAT view of the same
// memory it was rendered through, so it costs nothing extra. It holds the non linear clip
// space depth, 1 where nothing was drawn.
Texture2D           GBufferAlbedo               : register(t0);
Texture2D           GBufferNormal               : register(t1);
Texture2D           GBufferWorldPosition        : register(t2);
Texture2D           GBufferMaterialParams       : register(t3);
Texture2D           GBufferDepth                : register(t4);
Texture2DArray      ShadowDepth                 : register(t5);     // one depth slice per directional or area light
TextureCubeArray    ShadowDistance              : register(t6);     // six distance faces per point light
// t7 and t8 were the radiance and irradiance sky cubes, see LightPass::SKY_SRV_COUNT

SamplerState GBufferSampler     : register(s0);     // point, for the buffers above

// Stand in for the sky. Flat, and deliberately not black, so the background still reads
// as somewhere rather than as a hole in the image.
static const float3 BACKGROUND_COLOR = float3(0.08, 0.10, 0.14);

// EyePosition and InvViewProj come from CameraConstantBuffer in SharedTypes.h

// Direction this pixel looks at, found by pushing it out to the far plane and back
// through the view projection
float3 GetViewRay(float2 UV)
{
    float2 NDC      = UV * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 World    = mul(float4(NDC, 1.0, 1.0), InvViewProj);

    return normalize((World.xyz / World.w) - EyePosition.xyz);
}

float4 main(FullScreenPSInput Input) : SV_TARGET
{
    float4 AlbedoSample    = GBufferAlbedo.Sample(GBufferSampler, Input.UV);
    float3 Albedo           = AlbedoSample.xyz;
    float4 WorldPos        = GBufferWorldPosition.Sample(GBufferSampler, Input.UV);

    // BasePass marks written pixels with w of 1, everything else is background. Shading it
    // would be wrong anyway, its normal is zero and normalizing that would give NaN.
    if (WorldPos.w == 0.0)
    {
        return float4(BACKGROUND_COLOR, 1.0);
    }

    // A debug view of BasePass already decided the colour, shading it would hide whatever
    // is being inspected
    if (AlbedoSample.a == SURFACE_UNLIT)
    {
        return float4(Albedo, 1.0);
    }

    float3 Normal       = normalize(GBufferNormal.Sample(GBufferSampler, Input.UV)).xyz;
    float3 Params       = GBufferMaterialParams.Sample(GBufferSampler, Input.UV).xyz;

    float Metalness     = Params.r;

    // A material with no roughness map reads back as zero, which would be a perfect
    // mirror, so keep it off the very bottom of the range
    float Roughness     = max(Params.g, 0.04);
    float Occlusion     = Params.b;

    // Direct lighting from the lights placed in the scene, each one occluded by its own
    // slice of the shadow atlas
    float3 Direct       = ShadeSurface(ShadowDepth, ShadowDistance, GBufferSampler, Albedo, Normal, WorldPos.xyz, EyePosition.xyz, Roughness);

    // No image based lighting while the sky is out. ShadeReflections in Lighting.hlsl is
    // still there and takes its cubes as parameters, so restoring this is a call away.
    // Metalness and occlusion are read above and reach nothing yet, roughness now drives
    // the specular exponent.
    return float4(Direct, 1.0);
}
