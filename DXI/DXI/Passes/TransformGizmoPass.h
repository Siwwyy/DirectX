//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_TRANSFORMGIZMOPASS_H_INCLUDED
#define D3D12_TRANSFORMGIZMOPASS_H_INCLUDED

#include "PassBase.h"
#include "TransformGizmo.h"

// TransformGizmoConstants is defined in shaders/SharedTypes.h. WorldToCameraClip is already
// transposed for HLSL by the time it is written there.
//
// Root constants, like the other overlays: 160 bytes is 40 of the 64 DWORDs a root signature gets.
constexpr UINT TransformGizmoConstantCount = sizeof(TransformGizmoConstants) / sizeof(UINT32);

// Draws the handles that move or turn the selected object: three arrows, or three rings with the
// axis each turns about. The shape and the settings are TransformGizmo's, this only puts them on
// the screen.
//
// Everything is built out of SV_VertexID in the vertex shader, so it has no vertex buffer, no input
// layout and not even a per object buffer: the handles belong to no mesh. Thick lines are the same
// screen space quads the other overlays use.
//
// Drawn over everything, without a depth test. A handle that vanished behind the object it moves
// could not be grabbed.
class TransformGizmoPass : public PassBase
{
	DISABLE_COPY(TransformGizmoPass)

public:

	// Ctors & DCtor
	TransformGizmoPass();
	~TransformGizmoPass() = default;

	/**
	 * Init
	 */
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	/**
	 * Render | Draws the handles of the mode given. The caller binds the root signature, the pipeline
	 * and the view constants first.
	 */
	void RenderGizmo(DXGraphicsCommandList * const CommandList, const TransformMode Mode) noexcept;

	// The handles belong to no mesh, nothing is drawn per primitive or fullscreen
	void RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	void RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept override;

	// Root parameter index, assigned by the builder in Init so it cannot disagree with the
	// order the root signature was assembled in
	UINT ViewIdx	= 0;	// b1 | One TransformGizmoConstants, set as root constants
};

#endif // D3D12_TRANSFORMGIZMOPASS_H_INCLUDED
