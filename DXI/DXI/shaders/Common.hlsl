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
#if defined(BASE_PASS) || defined(SHADOW_PASS)
    // Matches the POSITION/TEXCOORD/NORMAL/TANGENT/BINORMAL input layout BasePass and
    // ShadowPass build their PSOs with. ShadowPass only reads the position out of it.
    //
    // The frame interpolates across the triangle. A mesh whose three corners carry the
    // same face normal still comes out flat, which is what the cube, the plane and a
    // loaded model do, so this costs them nothing. A mesh that supplies a real per vertex
    // normal gets smooth shading out of it.
    float4 Position                     : POSITION;
    float2 UV                           : TEXCOORD;
    float3 Normal                       : NORMAL;
    float4 Tangent                      : TANGENT;      // w is the bitangent handedness
    float4 Bitangent                    : BINORMAL;     // w unused
#else
    float4 Position                     : POSITION;
    #if USE_TEXCOORD
        float2 UV                       : TEXCOORD;
    #else
        float4 Color                    : COLOR;
    #endif
    float3 Normal                       : NORMAL;
#endif // BASE_PASS
};

// Pixel Shader input (and output of Vertex/Mesh shader)
struct PSInput
{
#ifdef BASE_PASS
    // BasePass writes the surface into the G-buffer, so it needs the material uv
    // and the world position on top of the normal. The tangent rides along so the
    // pixel shader can read a tangent space normal map.
    float4 Position                     : SV_POSITION;
    float4 WorldPosition               : POSITION;
    float2 UV                           : TEXCOORD;
    float3 Normal                       : NORMAL;       // interpolated, so the pixel shader renormalizes
    float4 Tangent                      : TANGENT;      // w is the bitangent handedness
    float4 Bitangent                    : BINORMAL;     // w unused
#else
    float4 Position                     : SV_POSITION;
    float4 WorldPosition               : POSITION;
    #if USE_TEXCOORD
        float2 UV                       : TEXCOORD;
    #else
        float4 Color                    : COLOR;
    #endif
    float3 Normal                       : NORMAL;
#endif // BASE_PASS

};


// GBufferOutput and the GBUFFER_* target indices live in SharedTypes.h, so the C++ enum,
// the SV_Target order and the SRV order in the light pass all come from one list.


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
struct FullScreenPSInput
{
    float4 Position                     : SV_POSITION;
    float2 UV                           : TEXCOORD;
};


#ifdef SHADOW_PASS

// PrimitiveConstantBuffer at b0 and ShadowViewConstants at b1 both come from
// SharedTypes.h, which declares them only for the passes that bind them. Only
// LocalToWorld is read here; the rest of the per object buffer belongs to the base pass.

// Output of the distance pass. A cube face needs the world position in the pixel shader,
// which a depth only pass never has to carry.
struct ShadowDistancePSInput
{
    float4 Position                     : SV_POSITION;
    float3 WorldPosition               : POSITION;
};

#endif // SHADOW_PASS
