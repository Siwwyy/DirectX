
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_MATH_H_INCLUDED
#define D3D12_MATH_H_INCLUDED

#include <DirectXMath.h>
#include "D3D12Helpers.h"

namespace Math
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};


	// Make sure Constant buffer is aligned to 256 bytes boundary
	// https://www.asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment
	struct alignas(256) ConstantBufferPerObject 
	{
		DirectX::XMFLOAT4X4 WorldViewProjectionMat4x4;
	};
	static_assert((sizeof(ConstantBufferPerObject) % 256) == 0, "Constant Buffer size must be aligned to 256-bytes boudary");

	struct alignas(256) CameraMatrices
	{
		DirectX::XMFLOAT4X4 cameraProjMat;			// this will store our projection matrix
		DirectX::XMFLOAT4X4 cameraViewMat;			// this will store our view matrix

		DirectX::XMFLOAT4	cameraPosition;			// this is our cameras position vector
		DirectX::XMFLOAT4	cameraTarget;			// a vector describing the point in space our camera is looking at
		DirectX::XMFLOAT4	cameraUp;				// the worlds up vector
	};

	struct alignas(256) CubeMatrices
	{
		DirectX::XMFLOAT4X4 cube1WorldMat;			// our first cubes world matrix (transformation matrix)
		DirectX::XMFLOAT4X4 cube1RotMat;			// this will keep track of our rotation for the first cube
		DirectX::XMFLOAT4	cube1Position;			// our first cubes position in space
	};
}

#endif // D3D12_MATH_H_INCLUDED