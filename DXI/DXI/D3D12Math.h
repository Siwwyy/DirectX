
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_MATH_H_INCLUDED
#define D3D12_MATH_H_INCLUDED

#include "pch.h"


// Vertex attributes
struct Vertex
{
	Vertex(float x, float y, float z, float r, float g, float b, float a) 
		: position(x, y, z)
		, color(r, g, b, a) {}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

struct TexVertex
{
	TexVertex(float x, float y, float z, float u, float v)
		: position(x, y, z)
		, texcoord(u,v) {}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texcoord;
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
	DirectX::XMFLOAT4X4 ProjMat;			// this will store our projection matrix
	DirectX::XMFLOAT4X4 ViewMat;			// this will store our view matrix

	DirectX::XMFLOAT4	Position;			// Position of the camera.
	DirectX::XMFLOAT4	Direction;			// N | Direction of the camera.
	DirectX::XMFLOAT4	Up;					// V | Up direction of the camera, typically < 0.0f, 1.0f, 0.0f >.
	DirectX::XMFLOAT4	Right;				// U | Right vector of camera
};

static_assert((sizeof(CameraMatrices) % 256) == 0, "CameraMatrices size must be aligned to 256-bytes boudary");
constexpr auto CameraMatricesSize		= sizeof(CameraMatrices);
constexpr auto CameraMatricesAlignment	= alignof(CameraMatrices);

// ObjectMatrices Matrices
struct alignas(256) ObjectMatrices
{
	ObjectMatrices();
	ObjectMatrices(const DirectX::XMFLOAT4 InitRot,
				   const DirectX::XMFLOAT4 InitPosition,
				   const DirectX::XMFLOAT4 InitScale);
	~ObjectMatrices() = default;

	// Membre variables
	DirectX::XMFLOAT4	RotVec;				// this will keep track of our rotation for object
	DirectX::XMFLOAT4	ScaleVec;			// scale vec
	DirectX::XMFLOAT4	PositionVec;		// position vector
	DirectX::XMFLOAT4X4 WorldMat;			// our world matrix (transformation matrix)
	
private:
	// Init Function
	void Init(const DirectX::XMFLOAT4 InitRot		= DX_IDENTITY_ROTATE4,
			  const DirectX::XMFLOAT4 InitPosition	= DX_IDENTITY_TRANSFORM4,
			  const DirectX::XMFLOAT4 InitScale		= DX_IDENTITY_SCALE4) noexcept;
};

static_assert((sizeof(ObjectMatrices) % 256) == 0, "ObjectMatrices size must be aligned to 256-bytes boudary");
constexpr auto ObjectMatricesSize		= sizeof(ObjectMatrices);
constexpr auto ObjectMatricesAlignment	= alignof(ObjectMatrices);


#endif // D3D12_MATH_H_INCLUDED