
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_MATH_H_INCLUDED
#define D3D12_MATH_H_INCLUDED

#include "pch.h"

//namespace Math
//{
	// Vertex attributes
	struct Vertex
	{
		Vertex(float x, float y, float z, float u, float v) : position(x, y, z), texcoord(u,v) {}
			//, color(r, g, b, a) {}


		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 texcoord;
		//DirectX::XMFLOAT4 color;
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