//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct CB
{
    float4 colorMultiplier;
};
ConstantBuffer<CB> cbObj : register(b0);

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    //result.color = color;
    //result.color = color * cbObj.colorMultiplier;
    result.color = color * cbObj.colorMultiplier;

    //position += float4(0.01, 0.01, 0.01, 0.01);

    return result;
}




float4 PSMain(PSInput input) : SV_TARGET
{
  //  #ifdef PIXEL_SHADER_ENABLED
		//return input.color;
  //  #endif
  //  return float4(0,0,0,0); //RGBA
    return input.color;
    //return input.color;

}

