
// Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "../Common.hlsl"

// Material of the primitive being drawn. The descriptor table covers every material slot,
// the registers follow the MaterialTextureSlot order of Material.h.
Texture2D MaterialDiffuse   : register(t0);
Texture2D MaterialNormal    : register(t1);
Texture2D MaterialSpecular  : register(t2);
Texture2D MaterialMetalness : register(t3);
Texture2D MaterialRoughness : register(t4);
Texture2D MaterialAO        : register(t5);

SamplerState MaterialSampler : register(s0);

// DebugMode comes from BasePassDebugConstants in SharedTypes.h, a root constant so
// switching views costs nothing per frame

/**
 * The debug views, selected by the DebugMode root constant.
 *
 * Most of them are derivatives. Pixels are always shaded in 2x2 quads, which is what lets
 * ddx and ddy exist: they subtract a value from its neighbour in the quad, giving how fast
 * it changes per pixel on screen, which is what the hardware itself uses to pick a mip.
 *
 * @param Velocity     Already computed by the caller, so the view shows exactly what the
 *                     velocity target holds rather than a second derivation of it
 * @param DebugColor   The colour to show, written only when the return value is true
 * @return             True when a view took over this pixel, in which case the caller
 *                     flags it SURFACE_UNLIT so LightPass leaves the colour alone
 */
bool GetDerivativeDebugColor(float2 UV, float3 WorldPosition, float3 Normal, float2 Velocity, out float3 DebugColor)
{
    DebugColor = float3(0.0, 0.0, 0.0);

    // ddx is the change across the quad horizontally, ddy vertically
    float2 UvDdx = ddx(UV);
    float2 UvDdy = ddy(UV);

    switch (DebugMode)
    {
    case BASE_PASS_DEBUG_UV_DENSITY:
    {
        // Red is how far the uv travels across one pixel horizontally, green vertically.
        // Both channels measure the same thing along the two screen axes, so the colour
        // reads directly: bright means a lot of texture packed into few pixels, and a
        // strong red or green tint means it is stretched more along that axis.
        // These are the same two lengths the mip view below takes the larger of.
        DebugColor = saturate(float3(length(UvDdx), length(UvDdy), 0.0) * DEBUG_UV_SCALE);
        return true;
    }

    case BASE_PASS_DEBUG_UV_DDX:
    {
        // The derivative itself, nothing folded together. u lands in red and v in green,
        // and since both can be negative the whole thing is biased so mid grey is zero,
        // brighter is positive and darker is negative. ddy gets its own view below,
        // because four numbers do not fit in three channels.
        DebugColor = saturate(float3(UvDdx * DEBUG_UV_SCALE, 0.0) + DEBUG_SIGNED_BIAS);
        return true;
    }

    case BASE_PASS_DEBUG_UV_DDY:
    {
        DebugColor = saturate(float3(UvDdy * DEBUG_UV_SCALE, 0.0) + DEBUG_SIGNED_BIAS);
        return true;
    }

    case BASE_PASS_DEBUG_MIP_LEVEL:
    {
        // Same math the sampler runs. Scale the uv derivatives up to texels, take the
        // longer of the two axes, and the log2 of it is the mip that matches that density.
        uint TextureWidth, TextureHeight;
        MaterialDiffuse.GetDimensions(TextureWidth, TextureHeight);

        float2 TexelsX  = UvDdx * float2(TextureWidth, TextureHeight);
        float2 TexelsY  = UvDdy * float2(TextureWidth, TextureHeight);
        float  MaxSqr   = max(dot(TexelsX, TexelsX), dot(TexelsY, TexelsY));

        // 0.5 * log2 of the squared length is log2 of the length
        float MipLevel  = 0.5 * log2(max(MaxSqr, 1e-8));

        // Dark is mip 0 and fully textured, bright is minified down to nothing.
        // Negative means magnified past mip 0, shown as blue.
        DebugColor = MipLevel < 0.0
                        ? float3(0.0, 0.0, saturate(-MipLevel))
                        : saturate(MipLevel / DEBUG_MIP_RANGE).xxx;
        return true;
    }

    case BASE_PASS_DEBUG_WORLD_DERIVATIVES:
    {
        // fwidth is abs(ddx) + abs(ddy), so this is how many world units one pixel covers.
        // Surfaces seen edge on change fastest and light up.
        DebugColor = saturate(fwidth(WorldPosition) * DEBUG_WORLD_SCALE);
        return true;
    }

    case BASE_PASS_DEBUG_NORMAL_DERIVATIVES:
    {
        // The normal interpolates, but every mesh here still carries a per face normal,
        // so it is constant inside a triangle and this stays black except on the quads
        // straddling two triangles, which is exactly where faceting shows up. A mesh
        // supplying real per vertex normals would show a gradient across the face.
        DebugColor = saturate(fwidth(Normal));
        return true;
    }

    case BASE_PASS_DEBUG_VELOCITY:
    {
        // What the velocity target holds, scaled up to be visible. A uv step is a fraction
        // of the screen, so a fast pan is still only a few hundredths and would otherwise
        // read as black.
        //
        // Red is horizontal motion and green vertical, both biased around mid grey so the
        // sign survives: brighter than grey is one way, darker the other. A still camera
        // over a still scene is therefore flat grey, which is the useful property, since
        // any colour at all means something moved.
        DebugColor = float3(saturate(Velocity * DEBUG_VELOCITY_SCALE + DEBUG_SIGNED_BIAS), DEBUG_SIGNED_BIAS);
        return true;
    }

    default:
        return false;
    }
}

// Surface only, no lighting. LightPass shades what lands in these targets.
// SV_IsFrontFace is a pixel shader system value, so it comes in next to PSInput rather
// than inside it. For a single sided material it is always true, since the back faces
// never make it out of the rasterizer.
GBufferOutput main(PSInput Input, bool bIsFrontFace : SV_IsFrontFace)
{
    GBufferOutput Output;

    // A two sided surface keeps its back faces, and those point away from the viewer,
    // so their normal has to be flipped or the underside comes out unlit
    float3 Normal           = normalize(Input.Normal);
    float3 VertexBitangent = Input.Bitangent.xyz;
    if (!bIsFrontFace)
    {
        // The bitangent goes with it. It was built against the front facing normal, and
        // cross(-N, T) is -cross(N, T), so leaving it alone would mirror the map in v.
        Normal           = -Normal;
        VertexBitangent = -VertexBitangent;
    }

    // Normal mapping. The map stores a direction in tangent space, where z points out of
    // the surface, so it has to be rotated into world space by the basis the surface
    // carries. Tangent, bitangent and normal are the three axes of that basis.
    {
        // Re-orthogonalize, the interpolation and the normal flip above can leave the
        // tangent slightly off perpendicular
        float3 Tangent      = normalize(Input.Tangent.xyz - Normal * dot(Normal, Input.Tangent.xyz));

        // The bitangent comes in from the vertex rather than out of a cross product. Only
        // its normal component is removed, so whatever angle it makes with the tangent
        // survives. That angle is the reason for storing it: on sheared uvs the two axes
        // are not perpendicular, and squaring them up would put the error back.
        float3 Bitangent    = normalize(VertexBitangent - Normal * dot(Normal, VertexBitangent));

        // Unpack from the 0 to 1 the texture holds back to a signed direction
        float3 TangentNormal = MaterialNormal.Sample(MaterialSampler, Input.UV).xyz * 2.0 - 1.0;

        // A slot with no map reads as zero, which unpacks to (-1,-1,-1) and is not a
        // direction at all. The default material fills it with a flat normal, but a
        // material loaded without one still has to come out of here unchanged.
        if (dot(TangentNormal, TangentNormal) > 0.01)
        {
            float3x3 TangentToWorld = float3x3(Tangent, Bitangent, Normal);
            Normal = normalize(mul(normalize(TangentNormal), TangentToWorld));
        }
    }

    Output.Normal           = float4(Normal, 0.0);

    // w of 1 tells LightPass that geometry was written here, the cleared background keeps 0
    Output.WorldPosition    = float4(Input.WorldPosition.xyz, 1.0);

    // What the reflections are driven by. An empty material slot keeps a null descriptor
    // and reads back as zero, which is the right answer for metalness on a dielectric.
    float Metalness         = MaterialMetalness.Sample(MaterialSampler, Input.UV).r;
    float Roughness         = MaterialRoughness.Sample(MaterialSampler, Input.UV).r;
    float AmbientOcclusion = MaterialAO.Sample(MaterialSampler, Input.UV).r;

    Output.MaterialParams   = float4(Metalness, Roughness, AmbientOcclusion, 1.0);

    // Where this pixel was on the previous frame, as a step in uv.
    //
    // Both clip positions are divided by their own w, which is the whole reason they are
    // carried separately rather than reconstructed: the perspective divide is what turns a
    // rotation of the camera into the right amount of screen motion at every depth. The
    // y flip is the usual one, clip space counts up and uv counts down.
    //
    // Interpolating the clip positions and dividing per pixel is not the same as
    // interpolating the divided result, and this is the order that is correct.
    float2 CurrentUV        = (Input.CurrentClip.xy / Input.CurrentClip.w) * float2(0.5, -0.5) + 0.5;
    float2 PrevUV           = (Input.PrevClip.xy / Input.PrevClip.w) * float2(0.5, -0.5) + 0.5;

    // A vertex behind the eye on either frame has a w of zero or less and the divide is
    // meaningless. It still rasterizes, because the triangle it belongs to was clipped
    // rather than dropped, so the guard is not optional.
    Output.Velocity         = (Input.CurrentClip.w > 0.0 && Input.PrevClip.w > 0.0)
                                ? CurrentUV - PrevUV
                                : float2(0.0, 0.0);

    // A debug view replaces the surface colour and asks LightPass to leave it alone,
    // otherwise the lighting would modulate the very numbers being inspected
    float3 DebugColor;
    if (GetDerivativeDebugColor(Input.UV, Input.WorldPosition.xyz, Normal, Output.Velocity, DebugColor))
    {
        Output.Albedo = float4(DebugColor, SURFACE_UNLIT);
    }
    else
    {
        Output.Albedo = float4(MaterialDiffuse.Sample(MaterialSampler, Input.UV).xyz, SURFACE_LIT);
    }

    return Output;
}
