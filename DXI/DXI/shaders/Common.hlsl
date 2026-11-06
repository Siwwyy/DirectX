//Copyright, Damian Andrysiak 2025, All Rights Reserved.

// Which register holds what, shared by every pass. Mirrors ShaderRegisters.h on the root
// signature side. Included here because every pass shader includes this file.
#include "ShaderRegisters.hlsl"

// Structs and constants the CPU shares with the shaders, one definition compiled as both.
// Also included from this file because everything downstream includes this one.
#include "SharedTypes.h"

// Vertex Shader input
struct VSInput
{
//     float4 position                     : POSITION;
// #if USE_TEXCOORD
//     float2 tex_coord                    : TEXCOORD;
// #else
//     float4 color                        : COLOR;
// #endif
//     nointerpolation float3 normal       : NORMAL;

#if defined(BASE_PASS) || defined(SHADOW_PASS)
    // Matches the POSITION/TEXCOORD/NORMAL/TANGENT/BINORMAL input layout BasePass and
    // ShadowPass build their PSOs with. ShadowPass only reads the position out of it.
    //
    // The frame interpolates across the triangle. A mesh whose three corners carry the
    // same face normal still comes out flat, which is what the cube, the plane and a
    // loaded model do, so this costs them nothing. A mesh that supplies a real per vertex
    // normal gets smooth shading out of it.
    float4 position                     : POSITION;
    float2 tex_coord                    : TEXCOORD;
    float3 normal                       : NORMAL;
    float4 tangent                      : TANGENT;      // w is the bitangent handedness
    float4 bitangent                    : BINORMAL;     // w unused
#else
    float4 position                     : POSITION;
    #if USE_TEXCOORD
        float2 tex_coord                : TEXCOORD;
    #else
        float4 color                    : COLOR;
    #endif
    float3 normal                       : NORMAL;
#endif // BASE_PASS
};

// Pixel Shader input (and output of Vertex/Mesh shader)
struct PSInput
{
//     float4 position                     : SV_POSITION;
//     float4 world_position         : POSITION;
// #if USE_TEXCOORD
//     float2 tex_coord                    : TEXCOORD;
// #else
//     float4 color                        : COLOR;
// #endif
//    nointerpolation float3 normal        : NORMAL;

#ifdef BASE_PASS
    // BasePass writes the surface into the G-buffer, so it needs the material uv
    // and the world position on top of the normal. The tangent rides along so the
    // pixel shader can read a tangent space normal map.
    float4 position                     : SV_POSITION;
    float4 world_position               : POSITION;
    float2 tex_coord                    : TEXCOORD;
    float3 normal                       : NORMAL;       // interpolated, so the pixel shader renormalizes
    float4 tangent                      : TANGENT;      // w is the bitangent handedness
    float4 bitangent                    : BINORMAL;     // w unused
#else
    float4 position                     : SV_POSITION;
    float4 world_position               : POSITION;
    #if USE_TEXCOORD
        float2 tex_coord                : TEXCOORD;
    #else
        float4 color                    : COLOR;
    #endif
    float3 normal                       : NORMAL;
#endif // BASE_PASS

};


// Output of BasePass, one entry per G-buffer target.
// Mirrors GBufferTarget of GBuffer.h, the order is the SV_Target order.
struct GBufferOutput
{
    float4 Albedo           : SV_Target0;   // Surface colour out of the material diffuse slot
    float4 Normal           : SV_Target1;   // World space normal
    float4 WorldPosition    : SV_Target2;   // World space position, w marks that geometry was written here
    float4 MaterialParams   : SV_Target3;   // r metalness, g roughness, b ambient occlusion
};


// BASE_PASS_DEBUG_* and the SURFACE_* flags come from SharedTypes.h, since the panel and
// the base pass root constant both deal in the same values.

// Derivatives are tiny numbers, these only exist to bring them into a visible range
#define DEBUG_UV_SCALE                      200.0
#define DEBUG_WORLD_SCALE                   20.0
#define DEBUG_MIP_RANGE                     8.0

// Derivatives are signed, the albedo target is UNORM and clamps anything negative to
// black. Remapping around this puts zero at mid grey so the sign stays visible.
#define DEBUG_SIGNED_BIAS                   0.5



// Input of a fullscreen pass, LightPass builds it straight out of SV_VertexID
struct FullscreenPSInput
{
    float4 position                     : SV_POSITION;
    float2 tex_coord                    : TEXCOORD;
};


#ifdef SHADOW_PASS

// PrimitiveConstantBuffer at b0 and ShadowViewConstants at b1 both come from
// SharedTypes.h, which declares them only for the passes that bind them. Only
// LocalToWorld is read here; the rest of the per object buffer belongs to the base pass.

// Output of the distance pass. A cube face needs the world position in the pixel shader,
// which a depth only pass never has to carry.
struct ShadowDistancePSInput
{
    float4 position                     : SV_POSITION;
    float3 world_position               : POSITION;
};

#endif // SHADOW_PASS