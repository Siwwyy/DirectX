
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_MATH_H_INCLUDED
#define D3D12_MATH_H_INCLUDED

#include "pch.h"

//namespace Math
//{
	// Vertex attributes
	struct Vertex
	{
		Vertex(float x, float y, float z, float u, float v) 
			: position(x, y, z)
			, texcoord(u,v) {}
			//, color(r, g, b, a) {}


		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texcoord;
		//DirectX::XMFLOAT4 color;
	};


	// Font stuffs
	struct TextVertex 
	{
		TextVertex(float r, float g, float b, float a, float u, float v, float tw, float th, float x, float y, float w, float h) 
			: color(r, g, b, a)
			, texCoord(u, v, tw, th)
			, pos(x, y, w, h) {}

		XMFLOAT4 pos;
		XMFLOAT4 texCoord;
		XMFLOAT4 color;
	};

	struct FontChar
	{
		// the unicode id
		int id;

		// these need to be converted to texture coordinates 
		// (where 0.0 is 0 and 1.0 is textureWidth of the font)
		float u; // u texture coordinate
		float v; // v texture coordinate
		float twidth; // width of character on texture
		float theight; // height of character on texture

		float width; // width of character in screen coords
		float height; // height of character in screen coords

		// these need to be normalized based on size of font
		float xoffset; // offset from current cursor pos to left side of character
		float yoffset; // offset from top of line to top of character
		float xadvance; // how far to move to right for next character
	};

	struct FontKerning
	{
		int firstid; // the first character
		int secondid; // the second character
		float amount; // the amount to add/subtract to second characters x
	};

	struct Font
	{
		std::wstring name; // name of the font
		std::wstring fontImage;
		int size; // size of font, lineheight and baseheight will be based on this as if this is a single unit (1.0)
		float lineHeight; // how far to move down to next line, will be normalized
		float baseHeight; // height of all characters, will be normalized
		int textureWidth; // width of the font texture
		int textureHeight; // height of the font texture
		int numCharacters; // number of characters in the font
		FontChar* CharList; // list of characters
		int numKernings; // the number of kernings
		FontKerning* KerningsList; // list to hold kerning values
		ID3D12Resource* textureBuffer; // the font texture resource
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle; // the font srv

		// these are how much the character is padded in the texture. We
		// add padding to give sampling a little space so it does not accidentally
		// padd the surrounding characters. We will need to subtract these paddings
		// from the actual spacing between characters to remove the gaps you would otherwise see
		float leftpadding;
		float toppadding;
		float rightpadding;
		float bottompadding;

		// this will return the amount of kerning we need to use for two characters
		float GetKerning(wchar_t first, wchar_t second)
		{
			for (int i = 0; i < numKernings; ++i)
			{
				if ((wchar_t)KerningsList[i].firstid == first && (wchar_t)KerningsList[i].secondid == second)
				{
					return KerningsList[i].amount;
				}
			}	
			return 0.0f;
		}

		// this will return a FontChar given a wide character
		FontChar* GetChar(wchar_t c)
		{
			for (int i = 0; i < numCharacters; ++i)
			{
				if (c == (wchar_t)CharList[i].id)
					return &CharList[i];
			}
			return nullptr;
		}
	};


	// Make sure Constant buffer is aligned to 256 bytes boundary
	// https://www.asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment
	struct alignas(256) ConstantBufferPerObject 
	{
		DirectX::XMFLOAT4X4 WorldViewProjectionMat4x4;
	};
	static_assert((sizeof(ConstantBufferPerObject) % 256) == 0, "Constant Buffer size must be aligned to 256-bytes boudary");
	constexpr auto ConstantBufferPerObjectSize		= sizeof(ConstantBufferPerObject);
	constexpr auto ConstantBufferPerObjectAlignment = alignof(ConstantBufferPerObject);


	// Camera matrices
	struct alignas(256) CameraMatrices
	{
		DirectX::XMFLOAT4X4 cameraProjMat;			// this will store our projection matrix
		DirectX::XMFLOAT4X4 cameraViewMat;			// this will store our view matrix

		DirectX::XMFLOAT4	cameraPosition;			// this is our cameras position vector
		DirectX::XMFLOAT4	cameraTarget;			// a vector describing the point in space our camera is looking at
		DirectX::XMFLOAT4	cameraUp;				// the worlds up vector
	};


	// Cube Matrices
	struct alignas(256) CubeMatrices
	{
		DirectX::XMFLOAT4X4 cubeWorldMat;			// our first cubes world matrix (transformation matrix)
		DirectX::XMFLOAT4X4 cubeRotMat;				// this will keep track of our rotation for the first cube
		DirectX::XMFLOAT4	cubePosition;			// our first cubes position in space
	};
//}

#endif // D3D12_MATH_H_INCLUDED