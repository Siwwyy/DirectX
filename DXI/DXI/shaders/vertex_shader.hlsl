
// //Copyright, Damian Andrysiak 2024, All Rights Reserved.

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
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