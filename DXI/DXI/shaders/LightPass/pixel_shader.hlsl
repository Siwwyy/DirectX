
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "../Common.hlsl"
#include "../Lighting.hlsl"

// G-buffer written by BasePass, registers follow the GBufferTarget order of GBuffer.h,
// then the shadow map and the two sky cubes after them
Texture2D GBufferAlbedo         : register(t0);
Texture2D GBufferNormal         : register(t1);
Texture2D GBufferWorldPosition  : register(t2);
Texture2D GBufferMaterialParams : register(t3);
Texture2DArray ShadowDepth      : register(t4);     // one depth slice per directional or area light
TextureCubeArray ShadowDistance : register(t5);     // six distance faces per point light
// t6 and t7 were the radiance and irradiance sky cubes, see LightPass::SKY_SRV_COUNT

SamplerState GBufferSampler     : register(s0);     // point, for the buffers above

// Stand in for the sky. Flat, and deliberately not black, so the background still reads
// as somewhere rather than as a hole in the image.
static const float3 BACKGROUND_COLOR = float3(0.08, 0.10, 0.14);

// EyePosition and InvViewProj come from CameraConstantBuffer in SharedTypes.h

// Direction this pixel looks at, found by pushing it out to the far plane and back
// through the view projection
float3 GetViewRay(float2 TexCoord)
{
    float2 ndc      = TexCoord * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 world    = mul(float4(ndc, 1.0, 1.0), InvViewProj);

    return normalize((world.xyz / world.w) - EyePosition.xyz);
}

float4 main(FullscreenPSInput input) : SV_TARGET
{
    float4 albedo_sample = GBufferAlbedo.Sample(GBufferSampler, input.tex_coord);
    float3 albedo       = albedo_sample.xyz;
    float4 world_pos    = GBufferWorldPosition.Sample(GBufferSampler, input.tex_coord);

    // BasePass marks written pixels with w of 1, everything else is background. Shading it
    // would be wrong anyway, its normal is zero and normalizing that would give NaN.
    if (world_pos.w == 0.0)
    {
        return float4(BACKGROUND_COLOR, 1.0);
    }

    // A debug view of BasePass already decided the colour, shading it would hide whatever
    // is being inspected
    if (albedo_sample.a == SURFACE_UNLIT)
    {
        return float4(albedo, 1.0);
    }

    float3 normal       = GBufferNormal.Sample(GBufferSampler, input.tex_coord).xyz;
    float3 params       = GBufferMaterialParams.Sample(GBufferSampler, input.tex_coord).xyz;

    float metalness     = params.r;

    // A material with no roughness map reads back as zero, which would be a perfect
    // mirror, so keep it off the very bottom of the range
    float roughness     = max(params.g, 0.04);
    float occlusion     = params.b;

    // Direct lighting from the lights placed in the scene, each one occluded by its own
    // slice of the shadow atlas
    float3 direct       = ShadeSurface(ShadowDepth, ShadowDistance, GBufferSampler, albedo, normal, world_pos.xyz, EyePosition.xyz);

    // No image based lighting while the sky is out. ShadeReflections in Lighting.hlsl is
    // still there and takes its cubes as parameters, so restoring this is a call away.
    // Metalness, roughness and occlusion are read above and reach nothing for now.
    return float4(direct, 1.0);
}
