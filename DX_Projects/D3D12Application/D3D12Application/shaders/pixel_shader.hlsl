
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

// Output from vertex shader
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


float4 PSMain(VSOutput input) : SV_TARGET
{
    #ifdef PIXEL_SHADER_ENABLED
		return input.color;
    #endif
    return float4(0,0,0,0); //RGBA

    //return input.color;

}

