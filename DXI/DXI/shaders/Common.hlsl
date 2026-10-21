//Copyright, Damian Andrysiak 2025, All Rights Reserved.

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
    // Matches the POSITION/TEXCOORD/NORMAL/TANGENT input layout BasePass and ShadowPass
    // build their PSOs with. ShadowPass only reads the position out of it.
    float4 position                     : POSITION;
    float2 tex_coord                    : TEXCOORD;
    nointerpolation float3 normal       : NORMAL;
    nointerpolation float4 tangent      : TANGENT;      // w is the bitangent handedness
#else
    float4 position                     : POSITION;
    #if USE_TEXCOORD
        float2 tex_coord                : TEXCOORD;
    #else
        float4 color                    : COLOR;
    #endif
    nointerpolation float3 normal       : NORMAL;
#endif // BASE_PASS
};

// Pixel Shader input (and output of Vertex/Mesh shader)
struct PSInput
{
//     float4 position                     : SV_POSITION;
//     float4 pixel_world_position         : POSITION;
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
    float4 pixel_world_position         : POSITION;
    float2 tex_coord                    : TEXCOORD;
    nointerpolation float3 normal       : NORMAL;
    nointerpolation float4 tangent      : TANGENT;      // w is the bitangent handedness
#else
    float4 position                     : SV_POSITION;
    float4 pixel_world_position         : POSITION;
    #if USE_TEXCOORD
        float2 tex_coord                : TEXCOORD;
    #else
        float4 color                    : COLOR;
    #endif
   nointerpolation float3 normal        : NORMAL;
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


// What BasePass writes into the albedo target instead of the surface colour.
// Keep in sync with BasePassDebugMode of Passes/BasePass.h.
#define BASE_PASS_DEBUG_OFF                 0
#define BASE_PASS_DEBUG_UV_DENSITY          1
#define BASE_PASS_DEBUG_UV_DDX              2
#define BASE_PASS_DEBUG_UV_DDY              3
#define BASE_PASS_DEBUG_MIP_LEVEL           4
#define BASE_PASS_DEBUG_WORLD_DERIVATIVES   5
#define BASE_PASS_DEBUG_NORMAL_DERIVATIVES  6

// Derivatives are tiny numbers, these only exist to bring them into a visible range
#define DEBUG_UV_SCALE                      200.0
#define DEBUG_WORLD_SCALE                   20.0
#define DEBUG_MIP_RANGE                     8.0

// Derivatives are signed, the albedo target is UNORM and clamps anything negative to
// black. Remapping around this puts zero at mid grey so the sign stays visible.
#define DEBUG_SIGNED_BIAS                   0.5

// Albedo alpha doubles as a flag for LightPass. A debug view has already decided what
// colour the pixel should be, so shading it again would hide the very thing being looked at.
#define SURFACE_LIT                         1.0
#define SURFACE_UNLIT                       0.0


// Input of a fullscreen pass, LightPass builds it straight out of SV_VertexID
struct FullscreenPSInput
{
    float4 position                     : SV_POSITION;
    float2 tex_coord                    : TEXCOORD;
};


#ifdef SHADOW_PASS

// Per object matrices. Only LocalToWorld is used here, the light half comes separately
// because the same object is rendered from several lights within one frame.
cbuffer ConstantBufferPerObject : register(b0)
{
    float4x4 LocalToWorld;
    float4x4 WorldToClip;           // the camera, unused here
    float4x4 LocalToWorldNormal;    // unused here, depth and distance only
};

// The view being rendered, as root constants so switching views costs nothing
cbuffer ConstantBufferPerShadowView : register(b1)
{
    float4x4 WorldToLightClip;      // World straight into this light's clip space
    float3   LightPosition;         // World space, a cube face measures distance from it
    float    LightRange;            // Distance is stored divided by this
};

// Output of the distance pass. A cube face needs the world position in the pixel shader,
// which a depth only pass never has to carry.
struct ShadowDistancePSInput
{
    float4 position                     : SV_POSITION;
    float3 world_position               : POSITION;
};

#endif // SHADOW_PASS