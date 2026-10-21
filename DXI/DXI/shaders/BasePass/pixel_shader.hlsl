
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

// A root constant, so switching views costs nothing per frame
cbuffer ConstantBufferPerDebug : register(b1)
{
    uint DebugMode;     // One of BASE_PASS_DEBUG_*
};

// Pixels are always shaded in 2x2 quads, which is what lets ddx and ddy exist: they
// subtract a value from its neighbour in the quad. The result is how fast that value
// changes per pixel on screen, and it is what the hardware itself uses to pick a mip.
//
// Returns true when a debug view took over the pixel, along with the colour to show.
bool GetDerivativeDebugColor(float2 TexCoord, float3 WorldPosition, float3 Normal, out float3 DebugColor)
{
    DebugColor = float3(0.0, 0.0, 0.0);

    // ddx is the change across the quad horizontally, ddy vertically
    float2 UvDdx = ddx(TexCoord);
    float2 UvDdy = ddy(TexCoord);

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
        // The normal is nointerpolation, so it is constant inside a triangle and this
        // stays black except on the quads straddling two triangles, which is exactly
        // where faceting shows up.
        DebugColor = saturate(fwidth(Normal));
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
GBufferOutput main(PSInput input, bool is_front_face : SV_IsFrontFace)
{
    GBufferOutput output;

    // A two sided surface keeps its back faces, and those point away from the viewer,
    // so their normal has to be flipped or the underside comes out unlit
    float3 normal = normalize(input.normal);
    if (!is_front_face)
    {
        normal = -normal;
    }

    // Normal mapping. The map stores a direction in tangent space, where z points out of
    // the surface, so it has to be rotated into world space by the basis the surface
    // carries. Tangent, bitangent and normal are the three axes of that basis.
    {
        // Re-orthogonalize, the interpolation and the normal flip above can leave the
        // tangent slightly off perpendicular
        float3 tangent      = normalize(input.tangent.xyz - normal * dot(normal, input.tangent.xyz));
        float3 bitangent    = cross(normal, tangent) * input.tangent.w;

        // Unpack from the 0 to 1 the texture holds back to a signed direction
        float3 tangent_normal = MaterialNormal.Sample(MaterialSampler, input.tex_coord).xyz * 2.0 - 1.0;

        // A slot with no map reads as zero, which unpacks to (-1,-1,-1) and is not a
        // direction at all. The default material fills it with a flat normal, but a
        // material loaded without one still has to come out of here unchanged.
        if (dot(tangent_normal, tangent_normal) > 0.01)
        {
            float3x3 TangentToWorld = float3x3(tangent, bitangent, normal);
            normal = normalize(mul(normalize(tangent_normal), TangentToWorld));
        }
    }

    output.Normal           = float4(normal, 0.0);

    // w of 1 tells LightPass that geometry was written here, the cleared background keeps 0
    output.WorldPosition    = float4(input.pixel_world_position.xyz, 1.0);

    // What the reflections are driven by. An empty material slot keeps a null descriptor
    // and reads back as zero, which is the right answer for metalness on a dielectric.
    float metalness         = MaterialMetalness.Sample(MaterialSampler, input.tex_coord).r;
    float roughness         = MaterialRoughness.Sample(MaterialSampler, input.tex_coord).r;
    float ambient_occlusion = MaterialAO.Sample(MaterialSampler, input.tex_coord).r;

    output.MaterialParams   = float4(metalness, roughness, ambient_occlusion, 1.0);

    // A debug view replaces the surface colour and asks LightPass to leave it alone,
    // otherwise the lighting would modulate the very numbers being inspected
    float3 debug_color;
    if (GetDerivativeDebugColor(input.tex_coord, input.pixel_world_position.xyz, normal, debug_color))
    {
        output.Albedo = float4(debug_color, SURFACE_UNLIT);
    }
    else
    {
        output.Albedo = float4(MaterialDiffuse.Sample(MaterialSampler, input.tex_coord).xyz, SURFACE_LIT);
    }

    return output;
}
