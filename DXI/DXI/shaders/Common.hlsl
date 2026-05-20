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

#ifdef BASE_PASS
    float4 position                     : POSITION;
    float4 color                        : COLOR;
    nointerpolation float3 normal       : NORMAL;
#else
    float4 position                     : POSITION;
    #if USE_TEXCOORD
        float2 tex_coord                    : TEXCOORD;
    #else
        float4 color                        : COLOR;
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
    float4 position                     : SV_POSITION;
    float4 color                        : COLOR;
    nointerpolation float3 normal       : NORMAL;
#else
    float4 position                     : SV_POSITION;
    float4 pixel_world_position         : POSITION;
    #if USE_TEXCOORD
        float2 tex_coord                    : TEXCOORD;
    #else
        float4 color                        : COLOR;
    #endif
   nointerpolation float3 normal        : NORMAL;
#endif // BASE_PASS

};