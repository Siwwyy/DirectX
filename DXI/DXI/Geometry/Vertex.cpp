#include "Vertex.h"

using namespace DirectX;

VertexFactory::VertexFactory(const VertexType Type)
	: Type(Type)
{ }

TriangleFrame ComputeTriangleFrame(const XMFLOAT3& Position0,
								const XMFLOAT3& Position1,
								const XMFLOAT3& Position2,
								const XMFLOAT2& uv0,
								const XMFLOAT2& uv1,
								const XMFLOAT2& uv2)
{
	const auto Pos0 = XMVectorSet(Position0.x, Position0.y, Position0.z, 0.f);
	const auto Pos1 = XMVectorSet(Position1.x, Position1.y, Position1.z, 0.f);
	const auto Pos2 = XMVectorSet(Position2.x, Position2.y, Position2.z, 0.f);

	const auto Edge1 = XMVectorSubtract(Pos1, Pos0);
	const auto Edge2 = XMVectorSubtract(Pos2, Pos0);

	const auto Normal = XMVector3Normalize(XMVector3Cross(Edge1, Edge2));
	DXASSERT(!XMVector3NearEqual(Normal, XMVectorZero(), XMVectorSplatEpsilon()), "Normal vector can not be zero vector");

	// How far the uv travels along each edge
	const float DeltaU1 = uv1.x - uv0.x;
	const float DeltaV1 = uv1.y - uv0.y;
	const float DeltaU2 = uv2.x - uv0.x;
	const float DeltaV2 = uv2.y - uv0.y;

	// Solving the two edges against those deltas gives the direction U grows in.
	// A zero determinant means the triangle has no uv area, so there is nothing to
	// solve and any perpendicular will do.
	const float Determinant		= (DeltaU1 * DeltaV2) - (DeltaU2 * DeltaV1);
	const bool bDegenerateUv	= fabsf(Determinant) < 1e-8f;

	XMVECTOR Tangent;
	if (bDegenerateUv)
	{
		const auto Reference = fabsf(XMVectorGetY(Normal)) > 0.99f
								? XMVectorSet(1.f, 0.f, 0.f, 0.f)
								: XMVectorSet(0.f, 1.f, 0.f, 0.f);
		Tangent = XMVector3Normalize(XMVector3Cross(Reference, Normal));
	}
	else
	{
		const float InvDeterminant = 1.f / Determinant;

		Tangent = XMVectorScale(
			XMVectorSubtract(XMVectorScale(Edge1, DeltaV2),
							XMVectorScale(Edge2, DeltaV1)),
			InvDeterminant);

		// Gram-Schmidt, the tangent has to sit in the plane perpendicular to the normal
		Tangent = XMVector3Normalize(
			XMVectorSubtract(Tangent,
				XMVectorScale(Normal, XMVectorGetX(XMVector3Dot(Normal, Tangent)))));
	}

	// Which way the bitangent has to point, mirrored uvs flip it
	const auto SolvedBitangent = XMVectorScale(
		XMVectorSubtract(XMVectorScale(Edge2, DeltaU1),
						XMVectorScale(Edge1, DeltaU2)),
		bDegenerateUv ? 0.f : 1.f / Determinant);

	const float Handedness = XMVectorGetX(
		XMVector3Dot(XMVector3Cross(Normal, Tangent), SolvedBitangent)) < 0.f ? -1.f : 1.f;

	// Only the normal component is taken out, leaving whatever angle the bitangent makes
	// with the tangent intact, because on sheared uvs that angle is the thing a stored
	// bitangent exists to preserve. No uv area means nothing to solve, so it falls back
	// to the cross product.
	XMVECTOR StoredBitangent = bDegenerateUv ? XMVector3Cross(Normal, Tangent) : SolvedBitangent;

	StoredBitangent = XMVector3Normalize(
		XMVectorSubtract(StoredBitangent,
			XMVectorScale(Normal, XMVectorGetX(XMVector3Dot(Normal, StoredBitangent)))));

	TriangleFrame Frame = {};
	Frame.Normal	= XMFLOAT4(XMVectorGetX(Normal),			XMVectorGetY(Normal),			XMVectorGetZ(Normal),			0.f);
	Frame.Tangent	= XMFLOAT4(XMVectorGetX(Tangent),			XMVectorGetY(Tangent),			XMVectorGetZ(Tangent),			Handedness);
	Frame.Bitangent	= XMFLOAT4(XMVectorGetX(StoredBitangent),	XMVectorGetY(StoredBitangent),	XMVectorGetZ(StoredBitangent),	0.f);

	return Frame;
}
