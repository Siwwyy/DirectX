//Copyright, Damian Andrysiak 2025, All Rights Reserved.

// Vertex Shader input
struct VSInput
{
    float3 position     : POSITION;
#if USE_TEXCOORD
    float2 tex_coord    : TEXCOORD;
#else
    float4 color        : COLOR;
#endif
    float3 normal       : NORMAL;
};

// Pixel Shader input (and output of Vertex/Mesh shader)
struct PSInput
{
    float4 position     : SV_POSITION;
#if USE_TEXCOORD
    float2 tex_coord    : TEXCOORD;
#else
    float4 color        : COLOR;
#endif
    float3 normal       : NORMAL;
};