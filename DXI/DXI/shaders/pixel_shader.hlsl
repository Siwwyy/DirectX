
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

// Output from vertex shader
// struct VSOutput
// {
//     float4 position : SV_POSITION;
//     float4 color    : COLOR;
// };

// #include "Common.hlsl"

// float4 PSMain(PSInput input) : SV_TARGET
// {
//     return input.color;
// }


struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float4 color: COLOR;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    // return interpolated color
    return input.color;
}

// float4 PSMain(VSOutput input) : SV_TARGET
// {
//     #ifdef PIXEL_SHADER_ENABLED
// 		return input.color;
// 	#endif
//     return float4(0,0,0,0); //RGBA
// }


// Texture2D t1 : register(t0);
// SamplerState s1 : register(s0);

// struct VS_OUTPUT
// {
//     float4 pos: SV_POSITION;
//     float2 texCoord: TEXCOORD;
// };

// float4 PSMain(VS_OUTPUT input) : SV_TARGET
// {
//     // return interpolated color
//     return t1.Sample(s1, input.texCoord);
// }