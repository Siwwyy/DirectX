
// Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "../Common.hlsl"
#include "../Lighting.hlsl"

// G-buffer written by BasePass, registers follow the GBufferTarget order of GBuffer.h,
// then the shadow map and the two sky cubes after them
Texture2D GBufferAlbedo         : register(t0);
Texture2D GBufferNormal         : register(t1);
Texture2D GBufferWorldPosition  : register(t2);
Texture2D GBufferMaterialParams : register(t3);
Texture2D ShadowDepth           : register(t4);
TextureCube SkyRadiance         : register(t5);     // the sky itself, mipped for roughness
TextureCube SkyIrradiance       : register(t6);     // the sky gathered over the hemisphere

SamplerState GBufferSampler     : register(s0);     // point, for the buffers above
SamplerState SkySampler         : register(s1);     // linear, the cubes are direction lookups

cbuffer ConstantBufferPerCamera : register(b1)
{
    float4   EyePosition;   // World space position of the camera, Camera::GetPosition()
    float4x4 InvViewProj;   // Clip space back to world
};

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
    float3 albedo       = GBufferAlbedo.Sample(GBufferSampler, input.tex_coord).xyz;
    float4 world_pos    = GBufferWorldPosition.Sample(GBufferSampler, input.tex_coord);

    // BasePass marks written pixels with w of 1, everything else is background, which is
    // where the sky shows through. Shading it would be wrong anyway, its normal is zero
    // and normalizing that would give NaN.
    if (world_pos.w == 0.0)
    {
        return float4(SkyRadiance.Sample(SkySampler, GetViewRay(input.tex_coord)).xyz, 1.0);
    }

    float3 normal       = GBufferNormal.Sample(GBufferSampler, input.tex_coord).xyz;
    float3 params       = GBufferMaterialParams.Sample(GBufferSampler, input.tex_coord).xyz;

    float metalness     = params.r;

    // A material with no roughness map reads back as zero, which would be a perfect
    // mirror, so keep it off the very bottom of the range
    float roughness     = max(params.g, 0.04);
    float occlusion     = params.b;

    // Direct lighting from the lights placed in the scene
    float shadow        = GetShadowFactor(ShadowDepth, GBufferSampler, world_pos.xyz, normal);
    float3 direct       = ShadeSurface(albedo, normal, world_pos.xyz, EyePosition.xyz, shadow);

    // How many mips the radiance cube has decides how blurred the roughest reflection
    // can get. Asking the texture avoids a constant that has to be kept in sync.
    uint cube_width, cube_height, cube_mips;
    SkyRadiance.GetDimensions(0, cube_width, cube_height, cube_mips);

    // Reflected lighting from the sky, diffuse and specular kept apart
    float3 reflections  = ShadeReflections(SkyRadiance,
                                            SkyIrradiance,
                                            SkySampler,
                                            float(cube_mips),
                                            albedo,
                                            normal,
                                            world_pos.xyz,
                                            EyePosition.xyz,
                                            metalness,
                                            roughness,
                                            occlusion);

    return float4(direct + reflections, 1.0);
}
