
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

// #include "Common.hlsl"

// PSInput VSMain(in VSInput input)
// {
//     PSInput result;

//     result.position = float4(input.position, 1.f);
//     result.color = input.color;

//     return result;
// }

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color: COLOR;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float4 color: COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.color = input.color;
    return output;
}


// // Vertex Shader Input struct
// struct VSInput
// {
//     float3 position : SV_POSITION;
//     float4 color    : COLOR;
// };

// // Vertex Shader Output struct
// struct VSOutput
// {
//     float4 position : SV_POSITION;
//     float4 color    : COLOR;
// };

// VSOutput VSMain(VSInput input)
// {
//     VSOutput output;
//     output.position = float4(input.position, 1.f);
//     output.color    = input.color;
//     return output;
// }


// struct VS_INPUT
// {
//     float4 pos : POSITION;
//     float2 texCoord: TEXCOORD;
// };

// struct VS_OUTPUT
// {
//     float4 pos: SV_POSITION;
//     float2 texCoord: TEXCOORD;
// };

// cbuffer ConstantBuffer : register(b0)
// {
//     float4x4 wvpMat;
// };

// VS_OUTPUT VSMain(VS_INPUT input)
// {
//     VS_OUTPUT output;
//     output.pos = mul(input.pos, wvpMat);
//     output.texCoord = input.texCoord;
//     return output;
// }