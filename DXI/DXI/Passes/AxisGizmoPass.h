//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_AXISGIZMOPASS_H_INCLUDED
#define D3D12_AXISGIZMOPASS_H_INCLUDED

#include "PassBase.h"

// AxisGizmoConstants is defined in shaders/SharedTypes.h. WorldToCameraClip is already
// transposed for HLSL by the time it is written there.
//
// Root constants, like the shadow view and the tangent frame: 96 bytes is 24 of the 64 DWORDs
// a root signature gets.
constexpr UINT AxisGizmoConstantCount = sizeof(AxisGizmoConstants) / sizeof(UINT32);

// What the origin axes look like, picked in the debug panel. The pass's own input: the panel
// only edits it and the renderer hands it on.
struct AxisGizmoSettings
{
	static constexpr float	MIN_AXIS_LENGTH		= 0.05f;
	static constexpr float	MAX_AXIS_LENGTH		= 10.0f;
	static constexpr float	MIN_LINE_THICKNESS	= 1.0f;
	static constexpr float	MAX_LINE_THICKNESS	= 12.0f;
	static constexpr float	MIN_HEAD_SIZE		= 4.0f;
	static constexpr float	MAX_HEAD_SIZE		= 48.0f;

	// Master switch. Which objects it draws is up to each object's bShowOriginAxes.
	bool	bEnabled			= false;	// Off, the overlay costs nothing

	// An origin is usually inside its own mesh, so the default shows it through the surface
	// instead of hiding it behind the very thing it belongs to
	bool	bDepthTest			= false;

	// World units one arrow runs from the origin. The same for every object, whatever its scale.
	float	AxisLength			= 0.5f;

	// Pixels, not world units, so the arrows read the same at any distance from the camera
	float	LineThickness		= 3.0f;
	float	HeadSize			= 16.0f;
};

// Draws a 3D axis at the origin of an object: three arrows along the object's own X, Y and Z,
// red, green and blue, turned with the object so its rotation can be read off at a glance.
//
// Everything is built out of SV_VertexID in the vertex shader, 27 vertices for the lot, so it
// has no vertex buffer and no input layout. The thick shafts are the same screen space quads
// the tangent frame's vectors are, and the heads are triangles sized in pixels.
//
// An overlay like the tangent frame: it runs after the light pass, straight into the back
// buffer, and can test against the depth the base pass wrote.
class AxisGizmoPass : public PassBase
{
	DISABLE_COPY(AxisGizmoPass)

public:

	// Ctors & DCtor
	AxisGizmoPass();
	~AxisGizmoPass() = default;

	/**
	 * Init
	 */
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	/**
	 * Render | Draws the three arrows at one primitive's origin. The caller binds the
	 * primitive's constants and the view constants first. The primitive is not read, its matrix
	 * comes in through the constants, but the signature is the one every pass shares.
	 */
	void RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	void RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept override;

	// Same shaders and root signature, only with the depth test off. The test is part of the
	// pipeline state, so both exist up front.
	_NODISCARD FORCEINLINE DXPipelineState* GetOnTopPipelineState() const noexcept
	{
		return OnTopPSO.PipelineState.Get();
	}

	// Pipeline the settings ask for
	_NODISCARD FORCEINLINE DXPipelineState* GetPipelineStateFor(const bool bDepthTest) const noexcept
	{
		return bDepthTest ? GetPipelineState() : GetOnTopPipelineState();
	}

	// Root parameter indices, assigned by the builder in Init so they cannot disagree with the
	// order the root signature was assembled in
	UINT PrimitiveCBVIdx	= 0;	// b0 | Per object
	UINT ViewIdx			= 0;	// b1 | One AxisGizmoConstants, set as root constants

private:

	// Depth test disabled, the arrows show through whatever is in front of them
	GraphicsPSO		OnTopPSO;
};

#endif // D3D12_AXISGIZMOPASS_H_INCLUDED
