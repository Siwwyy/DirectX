
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

// Lighting shared by every pass that shades a surface.
// Include this and call ShadeSurface(), the lights constant buffer comes along with it.
// The shadow map itself is passed in, because only the pass that has it in its root
// signature can declare it.

#ifndef LIGHTING_HLSL_INCLUDED
#define LIGHTING_HLSL_INCLUDED

// Kinds of light, has to match LightType of LightTypes/LightBase.h
#define LIGHT_TYPE_DIRECTIONAL  0
#define LIGHT_TYPE_POINT        1
#define LIGHT_TYPE_AREA         2

// How many lights fit in the lights constant buffer.
// Keep this in sync with MAX_LIGHTS of LightTypes/LightBase.h
#define MAX_LIGHTS              4

// Value of ShadowLightIdx when no light casts a shadow.
// Keep this in sync with NO_SHADOW_CASTER of LightTypes/LightBase.h
#define NO_SHADOW_CASTER        -1

// TODO | Should come from the material Specular and Roughness slots instead of being fixed
#define SPECULAR_STRENGTH       0.5
#define SHININESS               32.0

// A single light of the scene.
// Mirrors LightConstants of LightTypes/LightBase.h, mind the 16 bytes rows when editing.
struct Light
{
    float3  Position;       // World space position | Point, Area
    float   Intensity;      // Brightness multiplier

    float3  Direction;      // Direction the light points at | Directional, Area normal
    float   Range;          // Distance the light reaches | Point

    float3  Color;          // Linear colour
    uint    Type;           // One of LIGHT_TYPE_*

    float2  Size;           // Width and height of the emitting surface | Area
    int     ShadowSlot;     // Slice this light owns in the shadow atlas, NO_SHADOW_CASTER when it does not cast
};

cbuffer ConstantBufferPerLights : register(b2)
{
    Light    Lights[MAX_LIGHTS];                // Every light placed in the scene
    float4x4 ShadowViewProj[MAX_LIGHTS];        // World to that light's clip space | Directional, Area
    uint     NumLights;                         // How many entries of Lights are filled
};

// Direction from the shaded pixel towards the light, plus how much of it reaches the pixel.
// Directional lights reach everything, the other types fade out with the distance.
float3 GetLightDirection(Light InLight, float3 PixelWorldPosition, out float Attenuation)
{
    if (InLight.Type == LIGHT_TYPE_DIRECTIONAL)
    {
        Attenuation = 1.0;
        return -InLight.Direction;
    }

    // TODO | An area light should integrate over its rectangle, for now its centre is treated
    // as a point light, so Size only widens the falloff instead of softening the shape
    float3 ToLight      = InLight.Position - PixelWorldPosition;
    float  Distance     = length(ToLight);
    float  Reach        = max(InLight.Range, 1e-4);

    Attenuation         = saturate(1.0 - (Distance / Reach));
    Attenuation        *= Attenuation;

    if (InLight.Type == LIGHT_TYPE_AREA)
    {
        // Only the front side of the rectangle emits
        Attenuation *= saturate(dot(InLight.Direction, -ToLight / max(Distance, 1e-4)));
    }

    return ToLight / max(Distance, 1e-4);
}

// Bias scaled by how side on the surface is to the light. A grazing surface spans more
// depth per texel, so it needs more slack before it stops shadowing itself.
float GetShadowBias(float3 Normal, float3 LightDir, float Scale)
{
    float NdotL = saturate(dot(normalize(Normal), LightDir));
    return max(0.0005, Scale * (1.0 - NdotL));
}

// Directional and area lights | One depth slice each, so the pixel is projected into the
// light's clip space and compared against what was closest from there.
// Samples a 3x3 neighbourhood so the edge is not a hard staircase.
float GetShadowFactor2D(Texture2DArray ShadowDepth,
                        SamplerState ShadowSampler,
                        uint LightIdx,
                        float3 WorldPosition,
                        float3 Normal)
{
    int Slot = Lights[LightIdx].ShadowSlot;
    if (Slot < 0)
    {
        return 1.0;
    }

    // Project the pixel into the light's clip space, then into [0,1] texture space
    float4 LightClip    = mul(float4(WorldPosition, 1.0), ShadowViewProj[LightIdx]);
    float3 LightNDC     = LightClip.xyz / LightClip.w;
    float2 ShadowUV     = LightNDC.xy * float2(0.5, -0.5) + 0.5;

    // Nothing outside the view was rendered into the slice, and nothing behind the light
    // is in front of anything, so both count as lit
    if (LightClip.w <= 0.0 || any(ShadowUV < 0.0) || any(ShadowUV > 1.0) || LightNDC.z > 1.0)
    {
        return 1.0;
    }

    float Bias = GetShadowBias(Normal, -Lights[LightIdx].Direction, 0.0025);

    uint Width, Height, Slices;
    ShadowDepth.GetDimensions(Width, Height, Slices);
    float2 TexelSize = 1.0 / float2(Width, Height);

    float Lit = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float3 SampleUV = float3(ShadowUV + float2(x, y) * TexelSize, float(Slot));
            float  Occluder = ShadowDepth.SampleLevel(ShadowSampler, SampleUV, 0).r;
            Lit += ((LightNDC.z - Bias) <= Occluder) ? 1.0 : 0.0;
        }
    }

    return Lit / 9.0;
}

// Point lights | Six faces of a cube holding the distance the light reached, so the
// lookup is one sample along the direction to the pixel. No projection, no face maths,
// the cube sampler picks the face that rendered that direction.
float GetShadowFactorCube(TextureCubeArray ShadowDistance,
                          SamplerState ShadowSampler,
                          uint LightIdx,
                          float3 WorldPosition,
                          float3 Normal)
{
    int Slot = Lights[LightIdx].ShadowSlot;
    if (Slot < 0)
    {
        return 1.0;
    }

    float3 ToPixel      = WorldPosition - Lights[LightIdx].Position;
    float  Distance     = length(ToPixel);
    float  Range        = max(Lights[LightIdx].Range, 1e-4);

    // Past the range the light contributes nothing anyway
    if (Distance >= Range)
    {
        return 1.0;
    }

    // Both sides of the compare are distance over range, which is what the face stored
    float Current       = Distance / Range;
    float Bias          = GetShadowBias(Normal, -ToPixel / max(Distance, 1e-4), 0.02) / Range;

    float Occluder      = ShadowDistance.SampleLevel(ShadowSampler, float4(ToPixel, float(Slot)), 0).r;

    return ((Current - Bias) <= Occluder) ? 1.0 : 0.0;
}

// Shadow for whichever light this is, picking the storage its type was rendered into
float GetShadowFactor(Texture2DArray ShadowDepth,
                      TextureCubeArray ShadowDistance,
                      SamplerState ShadowSampler,
                      uint LightIdx,
                      float3 WorldPosition,
                      float3 Normal)
{
    if (Lights[LightIdx].Type == LIGHT_TYPE_POINT)
    {
        return GetShadowFactorCube(ShadowDistance, ShadowSampler, LightIdx, WorldPosition, Normal);
    }

    return GetShadowFactor2D(ShadowDepth, ShadowSampler, LightIdx, WorldPosition, Normal);
}

// Reflectance of a dielectric looked at head on. Metals take their tint from the albedo
// instead, which is what makes them look like metal.
#define DIELECTRIC_F0           0.04

// How much light bounces off at a grazing angle, Schlick's approximation.
// Roughness caps it, a rough surface never gets a clean mirror edge.
float3 FresnelSchlickRoughness(float3 F0, float NdotV, float Roughness)
{
    float3 Ceiling = max(float3(1.0 - Roughness, 1.0 - Roughness, 1.0 - Roughness), F0);
    return F0 + (Ceiling - F0) * pow(saturate(1.0 - NdotV), 5.0);
}

// Light the surface picks up from the sky rather than from any one light, split into the
// two ways a surface reflects it:
//
//   Diffuse  | the sky gathered over the whole hemisphere around the normal, which is
//             what the irradiance cube already holds, so it is a single lookup
//   Specular | the sky along the mirror direction, with roughness choosing how blurred a
//             mip to read, since every smaller mip of the radiance cube is a blurrier sky
//
// Metals have no diffuse reflection at all, dielectrics keep nearly all of theirs.
float3 ShadeReflections(TextureCube Radiance,
                        TextureCube Irradiance,
                        SamplerState CubeSampler,
                        float RadianceMipCount,
                        float3 Albedo,
                        float3 Normal,
                        float3 WorldPosition,
                        float3 EyePosition,
                        float Metalness,
                        float Roughness,
                        float AmbientOcclusion)
{
    float3 SurfaceNormal    = normalize(Normal);
    float3 EyeDirection     = normalize(EyePosition - WorldPosition);
    float  NdotV            = saturate(dot(SurfaceNormal, EyeDirection));

    float3 F0               = lerp(float3(DIELECTRIC_F0, DIELECTRIC_F0, DIELECTRIC_F0), Albedo, Metalness);
    float3 Fresnel          = FresnelSchlickRoughness(F0, NdotV, Roughness);

    // Diffuse reflection
    float3 IrradianceColor  = Irradiance.Sample(CubeSampler, SurfaceNormal).rgb;
    float3 DiffuseWeight    = (1.0 - Fresnel) * (1.0 - Metalness);
    float3 DiffuseIBL       = IrradianceColor * Albedo * DiffuseWeight;

    // Specular reflection
    float3 ReflectDir       = reflect(-EyeDirection, SurfaceNormal);
    float  MipLevel         = Roughness * max(RadianceMipCount - 1.0, 0.0);
    float3 RadianceColor    = Radiance.SampleLevel(CubeSampler, ReflectDir, MipLevel).rgb;
    float3 SpecularIBL      = RadianceColor * Fresnel;

    // Occlusion dims what the surroundings can reach, not what a light shines at directly
    return (DiffuseIBL + SpecularIBL) * AmbientOcclusion;
}

// Accumulates every light of the scene over a single surface sample.
// Every light is shadowed by its own slice of the atlas, so the shadow storage is passed
// in rather than a single precomputed factor.
float3 ShadeSurface(Texture2DArray ShadowDepth,
                    TextureCubeArray ShadowDistance,
                    SamplerState ShadowSampler,
                    float3 Albedo,
                    float3 Normal,
                    float3 WorldPosition,
                    float3 EyePosition)
{
    float3 SurfaceNormal    = normalize(Normal);
    float3 EyeDirection     = normalize(EyePosition - WorldPosition);

    float3 Diffuse          = float3(0.0, 0.0, 0.0);
    float3 Specular         = float3(0.0, 0.0, 0.0);

    for (uint LightIdx = 0; LightIdx < NumLights; ++LightIdx)
    {
        Light CurrentLight  = Lights[LightIdx];

        float Attenuation   = 0.0;
        float3 LightDir     = GetLightDirection(CurrentLight, WorldPosition, Attenuation);

        // Every light has a slice of its own now, so each one is occluded independently.
        // Skipped when the light already reaches nothing here.
        if (Attenuation > 0.0)
        {
            Attenuation *= GetShadowFactor(ShadowDepth, ShadowDistance, ShadowSampler, LightIdx, WorldPosition, SurfaceNormal);
        }

        float3 Radiance     = CurrentLight.Color * CurrentLight.Intensity * Attenuation;

        // Diffuse | Lambert
        float Diff          = max(dot(SurfaceNormal, LightDir), 0.0);
        Diffuse            += Diff * Radiance;

        // Specular | Phong, mirror the light around the normal and compare it against the eye direction
        float3 ReflectDir   = reflect(-LightDir, SurfaceNormal);
        float Spec          = pow(max(dot(EyeDirection, ReflectDir), 0.0), SHININESS);
        Specular           += SPECULAR_STRENGTH * Spec * Radiance;
    }

    return (Diffuse + Specular) * Albedo;
}

#endif // LIGHTING_HLSL_INCLUDED
