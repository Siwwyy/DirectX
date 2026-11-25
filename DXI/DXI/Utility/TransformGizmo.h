//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_TRANSFORMGIZMO_H_INCLUDED
#define DXI_TRANSFORMGIZMO_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIMath.h"
#include "Primitive.h"
#include "Picking.h"

// The handles that move or turn the selected object, and the logic of using them with the mouse.
// This file is the interaction only, nothing here draws: TransformGizmoPass puts the handles on the
// screen from the same frame this builds, so what is drawn and what can be grabbed is one shape.
//
// Three arrows move the object along an axis, three rings turn it about one. Pointing at a handle
// lights it, pressing on it grabs it, and from then on the object follows the cursor until the
// button is released.

/**
 * What a handle does. The values are the shader's, from SharedTypes.h.
 */
enum class TransformMode : UINT
{
	Move	= TRANSFORM_GIZMO_MODE_MOVE,	// Arrows, along an axis
	Rotate	= TRANSFORM_GIZMO_MODE_ROTATE	// Rings, about an axis
};

/**
 * Which three axes the handles follow
 */
enum class TransformSpace : UINT
{
	World,	// Always the scene's X, Y and Z
	Local	// The object's own, turned with it
};

// How the tool looks and behaves, picked in the debug panel
struct TransformGizmoSettings
{
	static constexpr float	MIN_SIZE			= 0.04f;
	static constexpr float	MAX_SIZE			= 0.40f;
	static constexpr float	MIN_LINE_THICKNESS	= 1.0f;
	static constexpr float	MAX_LINE_THICKNESS	= 12.0f;
	static constexpr float	MIN_HEAD_SIZE		= 4.0f;
	static constexpr float	MAX_HEAD_SIZE		= 48.0f;

	bool			bEnabled		= true;
	TransformMode	Mode			= TransformMode::Move;
	TransformSpace	Space			= TransformSpace::World;

	// How big the handles are, as a share of the height of the screen, so they read the same
	// near the object and far from it instead of shrinking into a dot
	float			Size			= 0.12f;

	// Pixels, like the other overlays
	float			LineThickness	= 3.0f;
	float			HeadSize		= 16.0f;

	// How close the cursor has to be to a handle, in pixels, for it to count as pointing at it
	float			GrabRadius		= 10.0f;

	// Steps taken while the snap key is held
	float			MoveSnap		= 0.25f;	// World units
	float			RotateSnap		= 15.0f;	// Degrees
};

/**
 * What the gizmo needs to know about the camera, gathered once per use so the maths below
 * does not each rebuild it
 */
struct GizmoCamera
{
	DirectX::XMFLOAT4X4	View;
	DirectX::XMFLOAT4X4	Proj;
	DirectX::XMFLOAT4X4	ViewProj;
	DirectX::XMFLOAT2	Viewport	= { 1.f, 1.f };	// Pixels
	DirectX::XMFLOAT3	Position;
};

_NODISCARD GizmoCamera MakeGizmoCamera(DirectX::FXMMATRIX View, DirectX::CXMMATRIX Proj, const DirectX::XMFLOAT3& Position, const DirectX::XMFLOAT2& Viewport);

/**
 * The handles of one object, in world space: where they are, which way each axis runs, and how
 * long they are drawn
 */
struct GizmoFrame
{
	DirectX::XMFLOAT3	Origin;
	DirectX::XMFLOAT3	Axes[3];	// Unit length and at right angles to each other
	float				Length;		// World units from the origin to the tip of an arrow, and the radius of a ring
};

class TransformGizmo
{
public:

	TransformGizmo()	= default;
	~TransformGizmo()	= default;

	/**
	 * Where the handles of an object are, for the settings given
	 */
	_NODISCARD static GizmoFrame BuildFrame(const GizmoCamera& Camera, const Primitive& Target, const TransformGizmoSettings& Settings);

	/**
	 * Lights the handle the cursor is on, if any. Call it as the cursor moves while nothing is held.
	 * A held handle stays held, and what lies under the cursor meanwhile does not matter.
	 */
	void UpdateHover(const GizmoCamera& Camera, const Primitive& Target, const TransformGizmoSettings& Settings, const DirectX::XMFLOAT2& CursorPixels);

	/**
	 * Takes hold of the handle under the cursor.
	 *
	 * @return Whether there was one. A press that returns false is an ordinary click, for picking.
	 */
	_NODISCARD bool BeginDrag(const GizmoCamera& Camera, Primitive& Target, const TransformGizmoSettings& Settings, const DirectX::XMFLOAT2& CursorPixels);

	/**
	 * Moves or turns the object to where the cursor is, measured from where it was when grabbed so
	 * nothing drifts however long the drag runs.
	 *
	 * @param bSnap Round to the settings' step instead of following the cursor smoothly
	 */
	void Drag(const GizmoCamera& Camera, Primitive& Target, const TransformGizmoSettings& Settings, const DirectX::XMFLOAT2& CursorPixels, const bool bSnap);

	void EndDrag() noexcept;

	/**
	 * Forgets the hover, for when the cursor is somewhere the handles can not be pointed at
	 */
	void ClearHover() noexcept
	{
		HoveredAxis = TRANSFORM_GIZMO_AXIS_NONE;
	}

	// Getters | The axis is 0 to 2 for X, Y, Z, or TRANSFORM_GIZMO_AXIS_NONE
	_NODISCARD FORCEINLINE bool IsDragging() const noexcept
	{
		return ActiveAxis != TRANSFORM_GIZMO_AXIS_NONE;
	}
	_NODISCARD FORCEINLINE UINT GetHoveredAxis() const noexcept
	{
		return HoveredAxis;
	}
	_NODISCARD FORCEINLINE UINT GetActiveAxis() const noexcept
	{
		return ActiveAxis;
	}

private:

	/**
	 * Which axis' handle is nearest the cursor within the grab radius, or none
	 */
	_NODISCARD static UINT FindAxisUnderCursor(const GizmoCamera& Camera, const GizmoFrame& Frame, const TransformGizmoSettings& Settings, const DirectX::XMFLOAT2& CursorPixels);

	UINT				HoveredAxis		= TRANSFORM_GIZMO_AXIS_NONE;
	UINT				ActiveAxis		= TRANSFORM_GIZMO_AXIS_NONE;

	// Everything below is how things were at the moment of the press. A drag is always measured
	// against this, so it never accumulates error.
	TransformMode		DragMode		= TransformMode::Move;
	DirectX::XMFLOAT3	StartPosition	= { 0.f, 0.f, 0.f };
	DirectX::XMFLOAT3	StartRotation	= { 0.f, 0.f, 0.f };	// Degrees
	DirectX::XMFLOAT3	StartScale		= { 1.f, 1.f, 1.f };
	DirectX::XMFLOAT3	DragOrigin		= { 0.f, 0.f, 0.f };	// The object's position, the centre of the handles
	DirectX::XMFLOAT3	DragAxis		= { 1.f, 0.f, 0.f };	// The axis in the world, fixed for the whole drag
	DirectX::XMFLOAT3	PlaneU			= { 0.f, 1.f, 0.f };	// Rotation | two directions in the ring's plane that
	DirectX::XMFLOAT3	PlaneV			= { 0.f, 0.f, 1.f };	// measure the angle against, with V = Axis x U

	// A grab needs the cursor to land on the axis or the ring's plane, which a handle seen edge on
	// never allows. The press is kept anyway and the first usable position becomes the grab.
	bool				bHasGrab		= false;
	float				GrabParam		= 0.f;					// Move | Where along the axis the cursor was
	float				LastAngle		= 0.f;					// Rotate | Angle of the cursor in the plane, last frame
	float				TotalAngle		= 0.f;					// Rotate | Turned so far, in radians, past full turns too
};

#endif // DXI_TRANSFORMGIZMO_H_INCLUDED
