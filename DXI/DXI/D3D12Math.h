//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_MATH_H_INCLUDED
#define D3D12_MATH_H_INCLUDED

#include "pch.h"
#include <array> // for fixed-size array helpers

// Vertex attributes
struct Vertex1
{
	constexpr Vertex1(float x, float y, float z, float r, float g, float b, float a)
		: position(x, y, z)
		, color(r, g, b, a) {}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

struct TexVertex
{
	constexpr TexVertex(float x, float y, float z, float u, float v)
		: position(x, y, z)
		, texcoord(u,v) {}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texcoord;
};

struct TexNormalVertex
{
	TexNormalVertex() = default;
	~TexNormalVertex() = default;
	constexpr TexNormalVertex(float x, float y, float z, float u, float v, float x1, float y1, float z1)
		: position(x, y, z)
		, texcoord(u, v) 
		, normal(x1, y1, z1)
	{
	}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texcoord;
	DirectX::XMFLOAT3 normal;
};

// Convert an N-element C-style array of TexVertex to an N-element std::array of TexNormalVertex.
// The function preserves element count at compile-time (N).
// Currently sets normals to (0,0,0) as a placeholder; replace normal computation as needed.
// Function calculates Face Normals for each triangle.
template<size_t N, size_t M>
std::array<TexNormalVertex, N> CreateFaceNormal123(const TexVertex (&TexVertexArray)[N], const DWORD (&IndicesList)[M])
{
	constexpr size_t NumVertices			= N;	// Number of vertices in the input array
	constexpr size_t NumVerticesPerTriangle = 3;	// Number of vertices in triangle (everytime its 3, but who knows?)
	constexpr size_t NumTriangles			= M / NumVerticesPerTriangle; // Assuming each triangle is defined by 3 indices
	std::array<TexNormalVertex, N> out{};

	for(size_t i = 0; i < NumTriangles; ++i)
	{
		// Get values
		const DWORD IdxX0			= IndicesList[i * NumVerticesPerTriangle + 0];
		const DWORD IdxX1			= IndicesList[i * NumVerticesPerTriangle + 1];
		const DWORD IdxX2			= IndicesList[i * NumVerticesPerTriangle + 2];
		const TexVertex& Vertex0	= TexVertexArray[IdxX0];
		const TexVertex& Vertex1	= TexVertexArray[IdxX1];
		const TexVertex& Vertex2	= TexVertexArray[IdxX2];
		const DirectX::XMVECTOR VertexPos0 = DirectX::XMVectorSet(Vertex0.position.x, Vertex0.position.y, Vertex0.position.z, 0.0f);
		const DirectX::XMVECTOR VertexPos1 = DirectX::XMVectorSet(Vertex1.position.x, Vertex1.position.y, Vertex1.position.z, 0.0f);
		const DirectX::XMVECTOR VertexPos2 = DirectX::XMVectorSet(Vertex2.position.x, Vertex2.position.y, Vertex2.position.z, 0.0f);

		// Calculate normal using cross product of two edges of the triangle (assuming vertices are defined in a consistent winding order) see per Primitive Vertex List and Indices List
		// see https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-normals.html
		const DirectX::XMVECTOR Edge1	= DirectX::XMVectorSubtract(VertexPos1, VertexPos0);
		const DirectX::XMVECTOR Edge2	= DirectX::XMVectorSubtract(VertexPos2, VertexPos0);
		const DirectX::XMVECTOR Normal	= DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Edge1, Edge2));

#if 1
		//DXLOG("Triangle %zu: Normal = (%f, %f, %f)", i, XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal))
#endif 

		// SANITY CHECKS
		// Check if normal is not a zero vector
		// Create a zero vector to compare against
		XMVECTOR Epsilon	= XMVectorSplatEpsilon();
		XMVECTOR ZeroVector = XMVectorZero();

		// Perform the near equality check
		// The result is a control vector that can be tested
		bool IsNormalNearZero = XMVector3NearEqual(Normal, ZeroVector, Epsilon);
		DXASSERT(!IsNormalNearZero, "Normal vector can not be zero vector");

		// Check if normal length is equal to 1.0 (Unit Length Vector
		XMVECTOR UnitVector		= XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		bool IsNormalUnitLength = XMVector3NearEqual(XMVector3Length(Normal), XMVector3Length(UnitVector), Epsilon);
		DXASSERT(IsNormalUnitLength, "Normal vector should be unit length vector");

		// Assing values to out array
		out[IdxX0] = TexNormalVertex(Vertex0.position.x, Vertex0.position.y, Vertex0.position.z,
									 Vertex0.texcoord.x, Vertex0.texcoord.y,
									XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal));
		out[IdxX1] = TexNormalVertex(Vertex1.position.x, Vertex1.position.y, Vertex1.position.z,
									 Vertex1.texcoord.x, Vertex1.texcoord.y,
									 XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal));
		out[IdxX2] = TexNormalVertex(Vertex2.position.x, Vertex2.position.y, Vertex2.position.z,
									 Vertex2.texcoord.x, Vertex2.texcoord.y,
									 XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal));
	}
	// Return
	return out;
}

// Make sure Constant buffer is aligned to 256 bytes boundary
// https://www.asawicki.info/news_1726_secrets_of_direct3d_12_resource_alignment
struct alignas(256) ConstantBufferPerObject 
{
	DirectX::XMFLOAT4X4 LocalToWorld;	// Only with translation
	DirectX::XMFLOAT4X4 WorldToClip;	// Translation * View * Proj (Clip)
};
static_assert((sizeof(ConstantBufferPerObject) % 256) == 0, "Constant Buffer size must be aligned to 256-bytes boudary");
constexpr auto ConstantBufferPerObjectSize		= sizeof(ConstantBufferPerObject);
constexpr auto ConstantBufferPerObjectAlignment = alignof(ConstantBufferPerObject);

// Constant buffer per Camera/World (View/Projection)
struct alignas(256) ConstantBufferPerCamera
{
	//
	//DirectX::XMFLOAT4X4 WorldToView;   // World * View
	DirectX::XMFLOAT4X4 ViewToClip;    // View * Clip

	//
	//DirectX::XMFLOAT4X4 WorldToClip;   // World * View * Clip
};
static_assert((sizeof(ConstantBufferPerCamera) % 256) == 0, "Constant Buffer size must be aligned to 256-bytes boudary");
constexpr auto ConstantBufferPerCameraSize		= sizeof(ConstantBufferPerCamera);
constexpr auto ConstantBufferPerCameraAlignment = alignof(ConstantBufferPerCamera);

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
	ObjectMatrices(const DirectX::XMFLOAT3 InitRot,
				   const DirectX::XMFLOAT3 InitPosition,
				   const DirectX::XMFLOAT3 InitScale);
	~ObjectMatrices() = default;

	// Membre variables
	DirectX::XMFLOAT4	RotVec;								// this will keep track of our rotation for object
	DirectX::XMFLOAT4	ScaleVec;							// scale vec
	DirectX::XMFLOAT4	PositionVec;						// position vector
	DirectX::XMFLOAT4X4 ScaleRotMat;						// our world matrix (transformation matrix scale+rot)
	DirectX::XMFLOAT4X4 WorldMat;							// our world matrix (transformation matrix scale+rot+translation)
	
	// Init Function
	void Transform(const XMFLOAT3 TranslateTransform	= DX_IDENTITY_TRANSLATE3,
					const XMFLOAT3 RotateTransform		= DX_IDENTITY_ROTATE3,
					const XMFLOAT3 ScaleTransform		= DX_IDENTITY_SCALE3) noexcept;

};

static_assert((sizeof(ObjectMatrices) % 256) == 0, "ObjectMatrices size must be aligned to 256-bytes boudary");
constexpr auto ObjectMatricesSize		= sizeof(ObjectMatrices);
constexpr auto ObjectMatricesAlignment	= alignof(ObjectMatrices);


#endif // D3D12_MATH_H_INCLUDED