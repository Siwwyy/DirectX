
// Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef SHADING_HLSL_INCLUDED
#define SHADING_HLSL_INCLUDED

#include "SharedTypes.h"




float GetLightAttenuation(Light Light, float3 WorldPosition, float3 LightPosition = float3(0.0, 0.0, 0.0))
{   
    if (Light.Type == LIGHT_TYPE_POINT)
    {
        float Distance = length(LightPosition - WorldPosition); // distance from pixel to light
        return 1.0 / (Distance * Distance);
    }
    else if (Light.Type == LIGHT_TYPE_AREA)
    {
        float3 ToSurface = WorldPosition - LightPosition;       // from the light out to the pixel
        float Distance = max(length(ToSurface), 1e-4);          // guarded, a pixel can sit on the light

        // Only the front of the rectangle emits, so anything behind it gets nothing.
        // Direction is the normal of the rectangle, see AreaLight
        float Facing = saturate(dot(Light.Direction.xyz, ToSurface / Distance));

        return Facing / (Distance * Distance);
    }
    else if (Light.Type == LIGHT_TYPE_SPOT)
    {
        float3 ToSurface = WorldPosition - LightPosition;       // from the light out to the pixel
        float Distance = max(length(ToSurface), 1e-4);

        // How far off the cone axis the pixel sits. A cosine grows as the angle shrinks, so
        // outer is the low end, and smoothstep fades the rim between the two angles instead
        // of cutting the cone off as a hard circle
        float CosAngle = dot(Light.Direction.xyz, ToSurface / Distance);
        float Cone = smoothstep(Light.SpotCosOuter, Light.SpotCosInner, CosAngle);

        return Cone / (Distance * Distance);
    }

    return 1.0; // if directional light, no attenuation
}


float3 ShadeSurfaceDXI(float3 Albedo,
                        float3 Normal,
                        float3 WorldPosition,
                        float3 ViewPosition)
{
    // Surface normal and view direction
    float3 SurfaceNormal = normalize(Normal);
    float3 ViewDirection = normalize(ViewPosition - WorldPosition); // view direction from pixel to camera

    // Accumulators for diffuse and specular lighting
    float3 DiffuseLighting  = float3(0.0, 0.0, 0.0);
    float3 SpecularLighting = float3(0.0, 0.0, 0.0);

    // Loop
    for (uint idx = 0; idx < NumLights; ++idx)
    {
        // Current light
        Light Light = Lights[idx];

        // Light color and intensity
        float3 Radiance = Light.Color * Light.Intensity * GetLightAttenuation(Light, WorldPosition, Light.Position.xyz);
        
        // Per light type calculations
        if (Light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            float3 LightDirection = normalize(-Light.Direction.xyz); // we need to invert light dir, to point from pixel to light

            // Diffuse lighting
            float3 Diffuse = max(0.0, dot(SurfaceNormal, LightDirection));
            DiffuseLighting += Diffuse * Radiance;

            // Specular lighting
            float3 Reflection = reflect(-LightDirection, SurfaceNormal); // reflection of the light direction off the surface normal
            float3 Specular = pow(max(0.0, dot(Reflection, ViewDirection)), 32.0); // specular lighting
            SpecularLighting += Specular * Radiance; // add the specular lighting to the total specular lighting
        }
        else if (Light.Type == LIGHT_TYPE_POINT)
        {
            float3 LightDirection = normalize(Light.Position.xyz - WorldPosition);

            // Diffuse lighting
            float3 Diffuse = max(0.0, dot(SurfaceNormal, LightDirection));
            DiffuseLighting += Diffuse * Radiance;

            // Specular lighting
            float3 Reflection = reflect(-LightDirection, SurfaceNormal); // reflection of the light direction off the surface normal
            float3 Specular = pow(max(0.0, dot(Reflection, ViewDirection)), 32.0); // specular lighting
            SpecularLighting += Specular * Radiance; // add the specular lighting to the total specular lighting
        }
        else if (Light.Type == LIGHT_TYPE_AREA)
        {
            // Treated as a point at the centre of the rectangle. Which side of it faces the
            // pixel is already in Radiance, Size does not soften the shape yet
            float3 LightDirection = normalize(Light.Position.xyz - WorldPosition);

            // Diffuse lighting
            float3 Diffuse = max(0.0, dot(SurfaceNormal, LightDirection));
            DiffuseLighting += Diffuse * Radiance;

            // Specular lighting
            float3 Reflection = reflect(-LightDirection, SurfaceNormal); // reflection of the light direction off the surface normal
            float3 Specular = pow(max(0.0, dot(Reflection, ViewDirection)), 32.0); // specular lighting
            SpecularLighting += Specular * Radiance; // add the specular lighting to the total specular lighting
        }
        else if (Light.Type == LIGHT_TYPE_SPOT)
        {
            // Same as a point light from here, the cone is already folded into Radiance
            float3 LightDirection = normalize(Light.Position.xyz - WorldPosition);

            // Diffuse lighting
            float3 Diffuse = max(0.0, dot(SurfaceNormal, LightDirection));
            DiffuseLighting += Diffuse * Radiance;

            // Specular lighting
            float3 Reflection = reflect(-LightDirection, SurfaceNormal); // reflection of the light direction off the surface normal
            float3 Specular = pow(max(0.0, dot(Reflection, ViewDirection)), 32.0); // specular lighting
            SpecularLighting += Specular * Radiance; // add the specular lighting to the total specular lighting
        }
    }
    
    

    return (DiffuseLighting + SpecularLighting) * Albedo;
}

#endif // SHADING_HLSL_INCLUDED
