
//Copyright, Damian Andrysiak 2026, All Rights Reserved.

// Lighting shared by every pass that shades a surface.
// Include this and call ShadeSurface(), the lights constant buffer comes along with it.
// The shadow map itself is passed in, because only the pass that has it in its root
// signature can declare it.

#ifndef LIGHTING_HLSL_INCLUDED
#define LIGHTING_HLSL_INCLUDED

// LIGHT_TYPE_*, MAX_LIGHTS, NO_SHADOW_CASTER and the Light struct all come from
// SharedTypes.h, which Common.hlsl includes. One definition, compiled as both languages.

// TODO | Should come from the material Specular and Roughness slots instead of being fixed
#define SPECULAR_STRENGTH       0.5
#define SHININESS               32.0

// Lights, ShadowViewProj and NumLights come from LightsConstantBuffer in SharedTypes.h

// ---------------------------------------------------------------------------------------
//  Baseline light maths, part 1 of 3: direction and falloff, per light type.
//
//  BASELINE is defined by LightPass.cpp alongside LIGHT_PASS. Turn it off there to compile
//  the #else branch instead and supply your own.
//
//  Anything written in the #else branch has to keep these signatures, because ShadeSurface
//  and LightPass call them by name.
// ---------------------------------------------------------------------------------------
#ifdef BASELINE

// How much of a positional light reaches a point this far away.
//
// Light spreads over a sphere, and that sphere's area grows with the square of the
// distance, so the same energy is spread thinner by exactly 1/d squared. That is the
// physical falloff, but on its own it has two problems: it runs to infinity at the centre
// and it never quite reaches zero, so a light could never be bounded by its range.
//
// SourceRadius fixes the first. A real emitter has size, and once inside it the intensity
// stops climbing. The windowing term fixes the second by easing the curve down to exactly
// nothing at the range, which keeps the cut off invisible while still making it finite.
float GetDistanceAttenuation(Light InLight, float DistanceSqr)
{
    // Clamped inverse square
    float RadiusSqr     = max(InLight.SourceRadius * InLight.SourceRadius, 1e-4);
    float Attenuation   = 1.0 / max(DistanceSqr, RadiusSqr);

    // Window, falling off as the fourth power of the distance so it stays near 1 for most
    // of the range and drops away only towards the edge
    float RangeSqr      = max(InLight.Range * InLight.Range, 1e-4);
    float Ratio         = saturate(DistanceSqr / RangeSqr);
    float Window        = saturate(1.0 - (Ratio * Ratio));

    return Attenuation * Window * Window;
}

// Direction from the shaded pixel towards the light, plus how much of it reaches the pixel.
// Directional lights reach everything, the other types fade out with the distance.
float3 GetLightDirection(Light InLight, float3 PixelWorldPosition, out float Attenuation)
{
    if (InLight.Type == LIGHT_TYPE_DIRECTIONAL)
    {
        // Parallel rays from infinitely far away, nothing to spread out
        Attenuation = 1.0;
        return -InLight.Direction;
    }

    // TODO | An area light should integrate over its rectangle, for now its centre is treated
    // as a point light, so Size only widens the falloff instead of softening the shape
    float3 ToLight      = InLight.Position - PixelWorldPosition;
    float  DistanceSqr  = dot(ToLight, ToLight);
    float  Distance     = sqrt(max(DistanceSqr, 1e-8));

    Attenuation         = GetDistanceAttenuation(InLight, DistanceSqr);

    if (InLight.Type == LIGHT_TYPE_AREA)
    {
        // Only the front side of the rectangle emits
        Attenuation *= saturate(dot(InLight.Direction, -ToLight / Distance));
    }

    return ToLight / Distance;
}

#else // BASELINE

// Your falloff. InLight carries Range and SourceRadius; DistanceSqr is squared on purpose,
// so a square root is only needed if the maths actually wants one.
float GetDistanceAttenuation(Light InLight, float DistanceSqr)
{
    // TODO | Write your own falloff
    return 0.0;
}

// Your per type direction. Returns the unit vector from the shaded pixel *towards* the
// light, and writes how much of the light reaches that pixel into Attenuation.
//
//   LIGHT_TYPE_DIRECTIONAL | parallel rays, nothing to spread out
//   LIGHT_TYPE_POINT       | radiates from InLight.Position, bounded by InLight.Range
//   LIGHT_TYPE_AREA        | a rectangle at InLight.Position facing InLight.Direction,
//                            InLight.Size across, emitting from one side only
float3 GetLightDirection(Light InLight, float3 PixelWorldPosition, out float Attenuation)
{
    // TODO | Write your own per type direction and attenuation
    Attenuation = 0.0;
    return float3(0.0, 1.0, 0.0);
}

#endif // BASELINE

// ---------------------------------------------------------------------------------------
//  Baseline light maths, part 2 of 3: reading occlusion back out of the shadow atlas.
//
//  The other half of shadowing lives in the ShadowPass shaders, which fill the atlas and
//  are guarded by a BASELINE of their own, defined in ShadowPass.cpp. The two halves have
//  to agree on what the atlas holds, so a technique that changes one usually changes both:
//  the 2D slices hold depth in the light's clip space and the cube faces hold distance
//  divided by range. Change that and these lookups have to follow.
//
//  The #else branch reports everything as lit rather than everything as shadowed, so a
//  half written technique leaves a scene you can still see, which is the opposite of what
//  the lighting stubs above do: an unwritten shadow is no shadow, but unwritten lighting
//  genuinely is black.
// ---------------------------------------------------------------------------------------
#ifdef BASELINE

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

#else // BASELINE

// Your depth bias, if the technique needs one. Normal and LightDir are unit vectors and
// Scale is whatever the caller thought the right magnitude was for its storage.
float GetShadowBias(float3 Normal, float3 LightDir, float Scale)
{
    // TODO | Write your own bias
    return 0.0;
}

// Your lookup for a directional or area light. ShadowDepth is an array of 2D slices and
// Lights[LightIdx].ShadowSlot indexes it, or is NO_SHADOW_CASTER when the light casts none.
// ShadowViewProj[LightIdx] is the matrix the slice was rendered with.
//
// Return 1 for fully lit down to 0 for fully occluded.
float GetShadowFactor2D(Texture2DArray ShadowDepth,
                        SamplerState ShadowSampler,
                        uint LightIdx,
                        float3 WorldPosition,
                        float3 Normal)
{
    // TODO | Write your own 2D shadow lookup
    return 1.0;
}

// Your lookup for a point light. ShadowDistance is an array of cube maps, sampled with a
// direction rather than a projected uv, so there is no face maths to do. What each face
// holds is up to the ShadowPass pixel shader.
//
// Return 1 for fully lit down to 0 for fully occluded.
float GetShadowFactorCube(TextureCubeArray ShadowDistance,
                          SamplerState ShadowSampler,
                          uint LightIdx,
                          float3 WorldPosition,
                          float3 Normal)
{
    // TODO | Write your own cube shadow lookup
    return 1.0;
}

// Your entry point. ShadeSurface calls only this one, so it is the only signature that has
// to survive; the two above are a convenience you are free to drop.
float GetShadowFactor(Texture2DArray ShadowDepth,
                      TextureCubeArray ShadowDistance,
                      SamplerState ShadowSampler,
                      uint LightIdx,
                      float3 WorldPosition,
                      float3 Normal)
{
    // TODO | Dispatch to your own lookups, by Lights[LightIdx].Type
    return 1.0;
}

#endif // BASELINE

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

// ---------------------------------------------------------------------------------------
//  Baseline light maths, part 3 of 3: how the lights are accumulated over a surface.
//  Lambert for the diffuse term and Phong for the specular, which is where SPECULAR_STRENGTH
//  and SHININESS at the top of this file are used.
// ---------------------------------------------------------------------------------------
#ifdef BASELINE

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

#else // BASELINE

// Your accumulation. Loop LightIdx over NumLights, read Lights[LightIdx], and build up
// whatever response you want out of it.
//
//   GetShadowFactor(ShadowDepth, ShadowDistance, ShadowSampler, LightIdx, WorldPosition, SurfaceNormal)
//     returns 1 for fully lit down to 0 for fully occluded, and already picks the right
//     storage for the light type. Worth skipping when a light reaches nothing anyway.
//     Note that it is stubbed out by the same switch that brought you here, so it reports
//     everything lit until part 2 above is written too.
//   Lights[LightIdx].ShadowSlot is NO_SHADOW_CASTER when that light casts none.
//
// Normal arrives straight out of the G-buffer, so normalize it before using it.
float3 ShadeSurface(Texture2DArray ShadowDepth,
                    TextureCubeArray ShadowDistance,
                    SamplerState ShadowSampler,
                    float3 Albedo,
                    float3 Normal,
                    float3 WorldPosition,
                    float3 EyePosition)
{
    // TODO | Write your own light accumulation
    return float3(0.0, 0.0, 0.0);
}

float3 DirectionalLightPass(
    float3 Normal               /* Per pixel Normal vector, should be normalized */,
    float3 WorldPosition        /* Pixel/Fragment World Pos */,
    float3 ViewPosition         /* Camera World Pos */,
    Light DirectionalLight      /* Directional Light */
)
{
    // Direction of the light
	float3 LightDirection   = normalize(-DirectionalLight.Direction);
    
    // Direction from pixel to the viewer (eye/camera)
    float3 ViewDirection    = normalize(ViewPosition - WorldPosition);

    // Diffuse shading
    float DiffuseFactor     = max(dot(Normal, LightDirection), 0.0);

    // Specular shading
    float3 ReflectDirection = reflect(-LightDirection, Normal);
    float SpecularFactor    = pow(max(dot(ViewDirection, ReflectDirection), 0.0), DirectionalLight.Intensity);
    
    // combine results
    return DiffuseFactor + SpecularFactor + float3(0, 0, 0) /* Ambient Component */;
}

// https://learnopengl.com/Lighting/Light-casters
float3 LightPass(
    float3 Normal           /* Per pixel Normal vector, should be normalized */,
    float3 WorldPosition    /* Pixel/Fragment World Pos */,
    float3 ViewPosition     /* Camera World Pos */
)
{
	// Accumulated light contributions  
    float3 DiffuseSpecularAmbient = float3(0.0, 0.0, 0.0);

	// loop though all lights in the scene and accumulate their contributions
    for (uint LightIdx = 0; LightIdx < NumLights; ++LightIdx)
    {
        // Three light types so far
        if(Lights[LightIdx].Type == LIGHT_TYPE_DIRECTIONAL)
        {
            DiffuseSpecularAmbient += DirectionalLightPass(Normal, WorldPosition, ViewPosition, Lights[LightIdx]);
        }
        else if (Lights[LightIdx].Type == LIGHT_TYPE_POINT)
        {
            // TODO | Handle point light
        }
        else if (Lights[LightIdx].Type == LIGHT_TYPE_AREA)
        {
            // TODO | Handle spot light
        }
    }

    //return (Diffuse + Specular + float3(0.0, 0.0, 0.0) /* Ambient for now disabled */) * Albedo;
    return DiffuseSpecularAmbient;
}

#endif // BASELINE

#endif // LIGHTING_HLSL_INCLUDED