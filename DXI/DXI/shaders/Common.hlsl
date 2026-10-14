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
    // Matches the POSITION/TEXCOORD/NORMAL input layout BasePass and ShadowPass
    // build their PSOs with. ShadowPass only reads the position out of it.
    float4 position                     : POSITION;
    float2 tex_coord                    : TEXCOORD;
    nointerpolation float3 normal       : NORMAL;
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
    // and the world position on top of the normal
    float4 position                     : SV_POSITION;
    float4 pixel_world_position         : POSITION;
    float2 tex_coord                    : TEXCOORD;
    nointerpolation float3 normal       : NORMAL;
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


// Input of a fullscreen pass, LightPass builds it straight out of SV_VertexID
struct FullscreenPSInput
{
    float4 position                     : SV_POSITION;
    float2 tex_coord                    : TEXCOORD;
};