
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

    output.Albedo           = float4(MaterialDiffuse.Sample(MaterialSampler, input.tex_coord).xyz, 1.0);
    output.Normal           = float4(normal, 0.0);

    // w of 1 tells LightPass that geometry was written here, the cleared background keeps 0
    output.WorldPosition    = float4(input.pixel_world_position.xyz, 1.0);

    // What the reflections are driven by. An empty material slot keeps a null descriptor
    // and reads back as zero, which is the right answer for metalness on a dielectric.
    float metalness         = MaterialMetalness.Sample(MaterialSampler, input.tex_coord).r;
    float roughness         = MaterialRoughness.Sample(MaterialSampler, input.tex_coord).r;
    float ambient_occlusion = MaterialAO.Sample(MaterialSampler, input.tex_coord).r;

    output.MaterialParams   = float4(metalness, roughness, ambient_occlusion, 1.0);

    return output;
}
