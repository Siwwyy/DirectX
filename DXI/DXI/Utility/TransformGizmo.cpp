//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "TransformGizmo.h"

#include <cfloat>

namespace
{
	// Closest a point may be to the camera plane for it to have a place on the screen
	constexpr float MIN_CLIP_W = 1e-4f;

	// A ray this close to lying in a plane, as the sine of the angle between them, is not used to
	// read a position off it. The spot it hits is thrown across the plane by a fraction of a pixel.
	constexpr float MIN_PLANE_FACING = 0.05f;

	// The cursor ray against the line of an arrow, unusable when they are this near to parallel
	constexpr float MIN_LINE_SEPARATION = 1e-4f;

	// Pixels added to the distance of a handle that is awkward to use, see FindAxisUnderCursor
	constexpr float HANDLE_PREFERENCE_PIXELS = 4.f;

	/**
	 * Where a point of the world lands on the screen, in pixels from the top left.
	 *
	 * @return False behind the camera, where there is no such place
	 */
	bool ProjectToPixels(const GizmoCamera& Camera, const XMFLOAT3& World, XMFLOAT2& OutPixels)
	{
		const XMVECTOR Clip = XMVector4Transform(XMVectorSet(World.x, World.y, World.z, 1.f), XMLoadFloat4x4(&Camera.ViewProj));

		XMFLOAT4 C;
		XMStoreFloat4(&C, Clip);
		if (C.w < MIN_CLIP_W)
		{
			return false;
		}

		// To NDC, which runs -1 to 1 with y up, and from there to pixels with y down
		OutPixels.x = ((C.x / C.w) * 0.5f + 0.5f) * Camera.Viewport.x;
		OutPixels.y = ((-C.y / C.w) * 0.5f + 0.5f) * Camera.Viewport.y;
		return true;
	}

	// Shortest distance from a point to a segment, all in pixels
	float DistanceToSegment(const XMFLOAT2& P, const XMFLOAT2& A, const XMFLOAT2& B)
	{
		const float Dx = B.x - A.x;
		const float Dy = B.y - A.y;
		const float LengthSqr = (Dx * Dx) + (Dy * Dy);

		// A segment that collapsed to a point is the distance to that point
		float T = 0.f;
		if (LengthSqr > 1e-8f)
		{
			T = std::clamp((((P.x - A.x) * Dx) + ((P.y - A.y) * Dy)) / LengthSqr, 0.f, 1.f);
		}

		const float Cx = A.x + (Dx * T) - P.x;
		const float Cy = A.y + (Dy * T) - P.y;
		return sqrtf((Cx * Cx) + (Cy * Cy));
	}

	/**
	 * Where along an axis line the cursor's ray passes closest to it, as a distance from the line's origin.
	 *
	 * Two lines in space nearly never meet. The pair of points one on each that are nearest
	 * each other is what a drag follows: the point on the axis the cursor is aiming at.
	 *
	 * @return False when the ray runs along the axis, where every point of it is as close as any other
	 */
	bool ClosestParamOnAxis(const PickRay& Ray, const XMFLOAT3& AxisOrigin, const XMFLOAT3& AxisDirection, float& OutParam)
	{
		const XMVECTOR RayDirection	= XMLoadFloat3(&Ray.Direction);
		const XMVECTOR Axis			= XMLoadFloat3(&AxisDirection);
		const XMVECTOR ToRay		= XMVectorSubtract(XMLoadFloat3(&Ray.Origin), XMLoadFloat3(&AxisOrigin));

		// Both directions are unit length, so the squares are one and only the cosine between them is left
		const float B = XMVectorGetX(XMVector3Dot(RayDirection, Axis));
		const float D = XMVectorGetX(XMVector3Dot(RayDirection, ToRay));
		const float E = XMVectorGetX(XMVector3Dot(Axis, ToRay));

		const float Denominator = 1.f - (B * B);
		if (Denominator < MIN_LINE_SEPARATION)
		{
			return false;
		}

		OutParam = (E - (B * D)) / Denominator;
		return true;
	}

	// Rounds to the nearest multiple of a step
	float SnapTo(const float Value, const float Step)
	{
		return Step > 0.f ? roundf(Value / Step) * Step : Value;
	}
}

GizmoCamera MakeGizmoCamera(FXMMATRIX View, CXMMATRIX Proj, const XMFLOAT3& Position, const XMFLOAT2& Viewport)
{
	GizmoCamera Camera;
	XMStoreFloat4x4(&Camera.View,		View);
	XMStoreFloat4x4(&Camera.Proj,		Proj);
	XMStoreFloat4x4(&Camera.ViewProj,	View * Proj);
	Camera.Viewport	= Viewport;
	Camera.Position	= Position;
	return Camera;
}

GizmoFrame TransformGizmo::BuildFrame(const GizmoCamera& Camera, const Primitive& Target, const TransformGizmoSettings& Settings)
{
	GizmoFrame Frame;

	const auto Position	= Target.GetPosVector();
	Frame.Origin		= { Position.x, Position.y, Position.z };

	// The axes. The world's own are the identity. The object's own are what its rotation turns
	// them into, which are the rows of that matrix in the row vector convention.
	if (Settings.Space == TransformSpace::Local)
	{
		const auto Rotation = Target.GetRotVector();
		const XMMATRIX Matrix = RotationFromEulerDegrees({ Rotation.x, Rotation.y, Rotation.z });

		for (int AxisIdx = 0; AxisIdx < 3; ++AxisIdx)
		{
			XMStoreFloat3(&Frame.Axes[AxisIdx], XMVector3Normalize(Matrix.r[AxisIdx]));
		}
	}
	else
	{
		Frame.Axes[0] = { 1.f, 0.f, 0.f };
		Frame.Axes[1] = { 0.f, 1.f, 0.f };
		Frame.Axes[2] = { 0.f, 0.f, 1.f };
	}

	// A fixed share of the screen's height wherever the object is. The height of the view at a
	// distance is twice that distance over the projection's focal length, which is its [1][1] entry
	// as 1 over the tangent of half the field of view.
	const float Depth		= std::max(XMVectorGetZ(XMVector3Transform(XMLoadFloat3(&Frame.Origin), XMLoadFloat4x4(&Camera.View))), 0.05f);
	const float ViewHeight	= 2.f * Depth / std::max(Camera.Proj._22, 1e-4f);
	Frame.Length			= Settings.Size * ViewHeight;

	return Frame;
}

UINT TransformGizmo::FindAxisUnderCursor(const GizmoCamera& Camera, const GizmoFrame& Frame, const TransformGizmoSettings& Settings, const XMFLOAT2& CursorPixels)
{
	UINT	Best		= TRANSFORM_GIZMO_AXIS_NONE;
	float	BestDistance = Settings.GrabRadius;

	const XMVECTOR Origin = XMLoadFloat3(&Frame.Origin);

	// Which way the camera looks at the handles from, to tell two that land on the same pixels apart
	const XMVECTOR ToCamera = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&Camera.Position), Origin));

	for (UINT Axis = 0; Axis < 3; ++Axis)
	{
		float Distance = FLT_MAX;

		// How much the axis points at the camera, 1 straight at it and 0 square across the view
		const float Alignment = fabsf(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&Frame.Axes[Axis]), ToCamera)));

		if (Settings.Mode == TransformMode::Move)
		{
			// The arrow is a line from the centre to its tip
			XMFLOAT3 Tip;
			XMStoreFloat3(&Tip, XMVectorAdd(Origin, XMVectorScale(XMLoadFloat3(&Frame.Axes[Axis]), Frame.Length)));

			XMFLOAT2 RootPixels, TipPixels;
			if (ProjectToPixels(Camera, Frame.Origin, RootPixels) && ProjectToPixels(Camera, Tip, TipPixels))
			{
				Distance = DistanceToSegment(CursorPixels, RootPixels, TipPixels);
			}
		}
		else
		{
			// The ring is a loop in the plane the axis is the normal of, walked in short steps. The
			// same sixty four points the shader draws it with, so what is lit is what is drawn.
			const XMVECTOR U = XMLoadFloat3(&Frame.Axes[(Axis + 1) % 3]);
			const XMVECTOR V = XMLoadFloat3(&Frame.Axes[(Axis + 2) % 3]);

			XMFLOAT2 PreviousPixels = {};
			bool bHasPrevious		= false;

			for (UINT Step = 0; Step <= TRANSFORM_GIZMO_RING_SEGMENTS; ++Step)
			{
				const float Angle = (static_cast<float>(Step) / static_cast<float>(TRANSFORM_GIZMO_RING_SEGMENTS)) * XM_2PI;

				XMFLOAT3 Point;
				XMStoreFloat3(&Point, XMVectorAdd(Origin, XMVectorScale(XMVectorAdd(XMVectorScale(U, cosf(Angle)), XMVectorScale(V, sinf(Angle))), Frame.Length)));

				XMFLOAT2 Pixels;
				if (!ProjectToPixels(Camera, Point, Pixels))
				{
					bHasPrevious = false;
					continue;
				}

				if (bHasPrevious)
				{
					Distance = std::min(Distance, DistanceToSegment(CursorPixels, PreviousPixels, Pixels));
				}

				PreviousPixels	= Pixels;
				bHasPrevious	= true;
			}
		}

		// Where handles overlap on screen, the one that is easier to use wins. An arrow pointing at
		// the camera is a stub, a ring seen edge on is a line, and either is a poor thing to drag
		// next to one that is seen square. A few pixels' worth is enough to settle a tie without
		// ever taking the choice from a handle the cursor is clearly nearer to.
		const float Preference = (Settings.Mode == TransformMode::Move) ? Alignment : (1.f - Alignment);
		Distance += Preference * HANDLE_PREFERENCE_PIXELS;

		if (Distance < BestDistance)
		{
			BestDistance	= Distance;
			Best			= Axis;
		}
	}

	return Best;
}

void TransformGizmo::UpdateHover(const GizmoCamera& Camera, const Primitive& Target, const TransformGizmoSettings& Settings, const XMFLOAT2& CursorPixels)
{
	// A held handle stays the one lit, however the cursor wanders off it while dragging
	if (IsDragging())
	{
		return;
	}

	HoveredAxis = FindAxisUnderCursor(Camera, BuildFrame(Camera, Target, Settings), Settings, CursorPixels);
}

bool TransformGizmo::BeginDrag(const GizmoCamera& Camera, Primitive& Target, const TransformGizmoSettings& Settings, const XMFLOAT2& CursorPixels)
{
	const GizmoFrame Frame	= BuildFrame(Camera, Target, Settings);
	const UINT Axis			= FindAxisUnderCursor(Camera, Frame, Settings, CursorPixels);
	if (Axis == TRANSFORM_GIZMO_AXIS_NONE)
	{
		return false;
	}

	ActiveAxis	= Axis;
	HoveredAxis	= Axis;
	DragMode	= Settings.Mode;

	// How things are right now, which every later position of the drag is measured against
	const auto Position	= Target.GetPosVector();
	const auto Rotation	= Target.GetRotVector();
	const auto Scale	= Target.GetScaleVector();
	StartPosition		= { Position.x, Position.y, Position.z };
	StartRotation		= { Rotation.x, Rotation.y, Rotation.z };
	StartScale			= { Scale.x, Scale.y, Scale.z };

	// The axis is fixed here and kept, so turning the object does not turn the axis it is being turned about
	DragOrigin	= Frame.Origin;
	DragAxis	= Frame.Axes[Axis];
	PlaneU		= Frame.Axes[(Axis + 1) % 3];
	PlaneV		= Frame.Axes[(Axis + 2) % 3];

	bHasGrab	= false;
	GrabParam	= 0.f;
	LastAngle	= 0.f;
	TotalAngle	= 0.f;

	// Take the grab right away, unless the handle is edge on and the first move has to do it
	Drag(Camera, Target, Settings, CursorPixels, false);

	return true;
}

void TransformGizmo::Drag(const GizmoCamera& Camera, Primitive& Target, const TransformGizmoSettings& Settings, const XMFLOAT2& CursorPixels, const bool bSnap)
{
	if (!IsDragging())
	{
		return;
	}

	const PickRay Ray = BuildPickRay(CursorPixels, Camera.Viewport, XMLoadFloat4x4(&Camera.View), XMLoadFloat4x4(&Camera.Proj));

	if (DragMode == TransformMode::Move)
	{
		float Param = 0.f;
		if (!ClosestParamOnAxis(Ray, DragOrigin, DragAxis, Param))
		{
			return;
		}

		// The first position is the grab. From then on the object moves by how far the cursor
		// has gone along the axis since, so it does not jump to put its centre under the cursor.
		if (!bHasGrab)
		{
			GrabParam	= Param;
			bHasGrab	= true;
			return;
		}

		float Distance = Param - GrabParam;
		if (bSnap)
		{
			Distance = SnapTo(Distance, Settings.MoveSnap);
		}

		XMFLOAT3 NewPosition;
		XMStoreFloat3(&NewPosition, XMVectorAdd(XMLoadFloat3(&StartPosition), XMVectorScale(XMLoadFloat3(&DragAxis), Distance)));
		Target.SetAbsoluteTransform(NewPosition, StartRotation, StartScale);
		return;
	}

	// Rotate | Where the ray meets the plane of the ring says which way the cursor is from the
	// centre, and the angle that direction makes is what the object is turned by.
	const XMVECTOR Normal		= XMLoadFloat3(&DragAxis);
	const XMVECTOR RayDirection	= XMLoadFloat3(&Ray.Direction);
	const float Facing			= XMVectorGetX(XMVector3Dot(RayDirection, Normal));
	if (fabsf(Facing) < MIN_PLANE_FACING)
	{
		return;
	}

	const XMVECTOR Origin	= XMLoadFloat3(&DragOrigin);
	const XMVECTOR RayStart	= XMLoadFloat3(&Ray.Origin);
	const float Distance	= XMVectorGetX(XMVector3Dot(XMVectorSubtract(Origin, RayStart), Normal)) / Facing;
	if (Distance < 0.f)
	{
		return;	// the plane is behind the cursor's ray
	}

	const XMVECTOR FromCentre	= XMVectorSubtract(XMVectorAdd(RayStart, XMVectorScale(RayDirection, Distance)), Origin);
	const float Angle			= atan2f(XMVectorGetX(XMVector3Dot(FromCentre, XMLoadFloat3(&PlaneV))),
										XMVectorGetX(XMVector3Dot(FromCentre, XMLoadFloat3(&PlaneU))));

	if (!bHasGrab)
	{
		LastAngle	= Angle;
		TotalAngle	= 0.f;
		bHasGrab	= true;
		return;
	}

	// The step since last frame, the short way round. The angle itself jumps from pi to -pi as the
	// cursor crosses behind the centre, the difference must not, or the object would spin a whole turn.
	float Step = Angle - LastAngle;
	while (Step > XM_PI)
	{
		Step -= XM_2PI;
	}
	while (Step < -XM_PI)
	{
		Step += XM_2PI;
	}
	TotalAngle	+= Step;
	LastAngle	= Angle;

	float Applied = TotalAngle;
	if (bSnap)
	{
		Applied = SnapTo(Applied, XMConvertToRadians(Settings.RotateSnap));
	}

	// Turned about a direction of the world, after what it already had. Along a local axis of the
	// object that is the same as turning about that axis in its own space, so one formula serves both.
	const XMMATRIX NewRotation = RotationFromEulerDegrees(StartRotation) * XMMatrixRotationAxis(Normal, Applied);
	Target.SetAbsoluteTransform(StartPosition, EulerDegreesFromRotation(NewRotation), StartScale);
}

void TransformGizmo::EndDrag() noexcept
{
	ActiveAxis	= TRANSFORM_GIZMO_AXIS_NONE;
	bHasGrab	= false;
}
