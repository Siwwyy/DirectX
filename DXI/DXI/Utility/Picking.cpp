//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "Picking.h"

#include <cfloat>

namespace
{
	// How far a box is grown before a ray is tried against it. A flat mesh, the plane, has no
	// thickness at all, and a ray that has to land inside an infinitely thin slab never would.
	constexpr float BOX_PADDING = 1e-4f;

	// Rays closer to parallel with a triangle than this are called misses, the division by the
	// determinant below stops meaning anything long before it reaches zero
	constexpr float PARALLEL_EPSILON = 1e-9f;

	/**
	 * Slab test: the ray enters a box where it has crossed all three pairs of planes, and leaves
	 * where it first crosses one of them again, so it hits when the entry comes before the exit.
	 * The whole line is tried and only a hit in front of the origin counts.
	 */
	bool RayHitsBox(FXMVECTOR Origin, FXMVECTOR Direction, const XMFLOAT3& Min, const XMFLOAT3& Max)
	{
		XMFLOAT3 O, D;
		XMStoreFloat3(&O, Origin);
		XMStoreFloat3(&D, Direction);

		const float OriginValues[3]		= { O.x, O.y, O.z };
		const float DirectionValues[3]	= { D.x, D.y, D.z };
		const float MinValues[3]		= { Min.x - BOX_PADDING, Min.y - BOX_PADDING, Min.z - BOX_PADDING };
		const float MaxValues[3]		= { Max.x + BOX_PADDING, Max.y + BOX_PADDING, Max.z + BOX_PADDING };

		float Entry = -FLT_MAX;
		float Exit	= FLT_MAX;

		for (int Axis = 0; Axis < 3; ++Axis)
		{
			if (fabsf(DirectionValues[Axis]) < PARALLEL_EPSILON)
			{
				// Running alongside this pair of planes: inside them the whole way, or never
				if (OriginValues[Axis] < MinValues[Axis] || OriginValues[Axis] > MaxValues[Axis])
				{
					return false;
				}
				continue;
			}

			const float Inverse	= 1.f / DirectionValues[Axis];
			float T0			= (MinValues[Axis] - OriginValues[Axis]) * Inverse;
			float T1			= (MaxValues[Axis] - OriginValues[Axis]) * Inverse;
			if (T0 > T1)
			{
				std::swap(T0, T1);
			}

			Entry	= std::max(Entry, T0);
			Exit	= std::min(Exit, T1);
		}

		return Exit >= std::max(Entry, 0.f);
	}

	/**
	 * Moller Trumbore. Solves Origin + t * Direction = V0 + u * (V1 - V0) + v * (V2 - V0) for
	 * t, u and v at once, and the point is inside the triangle when u and v are not negative and
	 * add up to no more than one. No face is skipped, so the winding does not matter.
	 */
	bool RayHitsTriangle(FXMVECTOR Origin, FXMVECTOR Direction, FXMVECTOR V0, GXMVECTOR V1, HXMVECTOR V2, float& OutT)
	{
		const XMVECTOR Edge1	= XMVectorSubtract(V1, V0);
		const XMVECTOR Edge2	= XMVectorSubtract(V2, V0);
		const XMVECTOR P		= XMVector3Cross(Direction, Edge2);
		const float Determinant	= XMVectorGetX(XMVector3Dot(Edge1, P));

		if (fabsf(Determinant) < PARALLEL_EPSILON)
		{
			return false;
		}

		const float InverseDeterminant	= 1.f / Determinant;
		const XMVECTOR ToOrigin			= XMVectorSubtract(Origin, V0);

		const float U = XMVectorGetX(XMVector3Dot(ToOrigin, P)) * InverseDeterminant;
		if (U < 0.f || U > 1.f)
		{
			return false;
		}

		const XMVECTOR Q	= XMVector3Cross(ToOrigin, Edge1);
		const float V		= XMVectorGetX(XMVector3Dot(Direction, Q)) * InverseDeterminant;
		if (V < 0.f || (U + V) > 1.f)
		{
			return false;
		}

		const float T = XMVectorGetX(XMVector3Dot(Edge2, Q)) * InverseDeterminant;
		if (T < 0.f)
		{
			return false;	// behind the origin, the line meets it but the half line does not
		}

		OutT = T;
		return true;
	}
}

PickRay BuildPickRay(const XMFLOAT2& CursorPixels, const XMFLOAT2& ViewportSize, FXMMATRIX View, CXMMATRIX Proj)
{
	// Unproject takes the pixel and a depth in 0 to 1, near to far, to a point in the world. The same
	// pixel at the two ends of the range gives two points on the line of sight.
	const XMVECTOR NearPoint	= XMVector3Unproject(XMVectorSet(CursorPixels.x, CursorPixels.y, 0.f, 1.f),
													0.f, 0.f, ViewportSize.x, ViewportSize.y, 0.f, 1.f,
													Proj, View, XMMatrixIdentity());
	const XMVECTOR FarPoint		= XMVector3Unproject(XMVectorSet(CursorPixels.x, CursorPixels.y, 1.f, 1.f),
													0.f, 0.f, ViewportSize.x, ViewportSize.y, 0.f, 1.f,
													Proj, View, XMMatrixIdentity());

	PickRay Ray;
	XMStoreFloat3(&Ray.Origin,		NearPoint);
	XMStoreFloat3(&Ray.Direction,	XMVector3Normalize(XMVectorSubtract(FarPoint, NearPoint)));
	return Ray;
}

bool RayIntersectsPrimitive(const PickRay& Ray, const Primitive& Target, float& OutDistance)
{
	const auto& Positions	= Target.VertexFactory.CpuPositions;
	const auto& Indices		= Target.VertexFactory.CpuIndices;
	if (Positions.empty() || Indices.size() < 3)
	{
		return false;
	}

	// The triangles are in the object's own space and the ray is in the world's. Bringing the ray
	// over is one transform, taking every vertex the other way would be one per vertex.
	// The direction is left unnormalized on purpose: a scale stretches it, and the t that comes
	// out is then a distance along the original ray, in the world, whatever the scale is.
	const auto WorldMatrix		= Target.GetWorldMatrix();
	XMVECTOR Determinant;
	const XMMATRIX WorldToLocal	= XMMatrixInverse(&Determinant, XMLoadFloat4x4(&WorldMatrix));
	if (fabsf(XMVectorGetX(Determinant)) < 1e-12f)
	{
		return false;	// scaled flat to nothing on some axis, there is no surface to hit
	}

	const XMVECTOR LocalOrigin		= XMVector3TransformCoord(XMLoadFloat3(&Ray.Origin), WorldToLocal);
	const XMVECTOR LocalDirection	= XMVector3TransformNormal(XMLoadFloat3(&Ray.Direction), WorldToLocal);

	if (!RayHitsBox(LocalOrigin, LocalDirection, Target.VertexFactory.LocalBoundsMin, Target.VertexFactory.LocalBoundsMax))
	{
		return false;
	}

	float Closest	= FLT_MAX;
	const size_t TriangleCount = Indices.size() / 3;

	for (size_t TriangleIdx = 0; TriangleIdx < TriangleCount; ++TriangleIdx)
	{
		const TypeOfIndice I0 = Indices[(TriangleIdx * 3) + 0];
		const TypeOfIndice I1 = Indices[(TriangleIdx * 3) + 1];
		const TypeOfIndice I2 = Indices[(TriangleIdx * 3) + 2];
		if (I0 >= Positions.size() || I1 >= Positions.size() || I2 >= Positions.size())
		{
			continue;
		}

		float T = 0.f;
		if (RayHitsTriangle(LocalOrigin, LocalDirection,
							XMLoadFloat3(&Positions[I0]), XMLoadFloat3(&Positions[I1]), XMLoadFloat3(&Positions[I2]), T)
			&& T < Closest)
		{
			Closest = T;
		}
	}

	if (Closest == FLT_MAX)
	{
		return false;
	}

	OutDistance = Closest;
	return true;
}

int PickPrimitive(const PickRay& Ray, const std::vector<std::unique_ptr<Primitive>>& Primitives)
{
	int		Best		= -1;
	float	BestDistance = FLT_MAX;

	for (size_t PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
	{
		float Distance = 0.f;
		if (RayIntersectsPrimitive(Ray, *Primitives[PrimitiveIdx], Distance) && Distance < BestDistance)
		{
			BestDistance	= Distance;
			Best			= static_cast<int>(PrimitiveIdx);
		}
	}

	return Best;
}
