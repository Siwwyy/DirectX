//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_TANGENTFRAMEPASS_H_INCLUDED
#define D3D12_TANGENTFRAMEPASS_H_INCLUDED

#include "PassBase.h"

// TangentFrameConstants is defined in shaders/SharedTypes.h. WorldToCameraClip is already
// transposed for HLSL by the time it is written there.
//
// Root constants, like the shadow view: 80 bytes is 20 of the 64 DWORDs a root signature gets.
constexpr UINT TangentFrameConstantCount = sizeof(TangentFrameConstants) / sizeof(UINT32);

// Where ShowAllVertices sits among those constants, in DWORDs. It changes with every draw while
// the rest holds for the whole pass, so it is written on its own rather than the lot again.
constexpr UINT TangentFrameShowAllOffset = static_cast<UINT>(offsetof(TangentFrameConstants, ShowAllVertices) / sizeof(UINT32));

// What the overlay draws, picked in the debug panel. Lives here because it is the pass's
// own input, the panel only edits it and the renderer hands it on.
struct TangentFrameSettings
{
	// Limits the panel clamps to, so the shader never has to defend against a zero step or
	// a vector too short to see
	static constexpr float	MIN_VECTOR_LENGTH	= 0.01f;
	static constexpr float	MAX_VECTOR_LENGTH	= 5.0f;
	static constexpr float	MIN_LINE_THICKNESS	= 1.0f;
	static constexpr float	MAX_LINE_THICKNESS	= 12.0f;
	static constexpr float	MIN_VERTEX_PERCENT	= 0.1f;
	static constexpr float	MAX_VERTEX_PERCENT	= 100.0f;

	// Master switch. Which objects it draws is up to each object's bShowTangentFrame.
	bool	bEnabled			= false;	// Off, the overlay costs nothing
	bool	bShowTangent		= true;		// Red, along growing U
	bool	bShowBitangent		= true;		// Green, along growing V
	bool	bShowNormal			= true;		// Blue, out of the surface
	bool	bDepthTest			= true;		// Hidden behind geometry, or drawn over everything

	// World units one unit vector is drawn at. 1 draws them at their true length.
	float	VectorLength		= 0.25f;

	// How wide a line is drawn, in pixels. Not world units, so it reads the same at any
	// distance from the camera.
	float	LineThickness		= 2.0f;

	// Percent of a mesh's vertices that draw their vectors, picked by a fixed hash of the
	// vertex index so the same ones show every frame. A dense mesh buries itself in lines
	// at 100, so the default shows only a sample.
	float	VertexPercent		= 10.0f;

	/**
	 * @return The same share as a fraction of 1, the unit the shader compares against
	 */
	_NODISCARD float GetVertexFraction() const noexcept
	{
		return VertexPercent * 0.01f;
	}

	/**
	 * @return The bits of TANGENT_FRAME_SHOW_* for the vectors switched on
	 */
	_NODISCARD UINT GetVectorMask() const noexcept
	{
		return (bShowTangent	? TANGENT_FRAME_SHOW_TANGENT	: 0u)
			 | (bShowBitangent	? TANGENT_FRAME_SHOW_BITANGENT	: 0u)
			 | (bShowNormal		? TANGENT_FRAME_SHOW_NORMAL		: 0u);
	}
};

// Draws the surface basis of a mesh as lines (screen space quads, see LineThickness): tangent, bitangent and normal at every vertex,
// in world space and at unit length scaled by one setting, so a wrong frame is something to
// look at rather than infer from how the lighting went wrong.
//
// Each vertex becomes one instance of an eighteen vertex draw, three quads of six corners. The mesh's
// own vertex buffer is bound as per instance data for it, so the lines are built on the GPU
// from what the base pass reads and nothing is copied on the CPU.
//
// An overlay: it runs after the light pass, straight into the back buffer, with the depth
// the base pass wrote to hide the lines behind geometry.
class TangentFramePass : public PassBase
{
	DISABLE_COPY(TangentFramePass)

public:

	// Ctors & DCtor
	TangentFramePass();
	~TangentFramePass() = default;

	/**
	 * Init
	 */
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	/**
	 * Render | Draws the frame of every vertex of one primitive. The caller binds the
	 * primitive's constants and the view constants first. Whether to show every vertex is
	 * written here, it is the one value of the view constants that belongs to the primitive.
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

	// Root parameter indices, the caller binds them before Render. Assigned by the builder in
	// Init rather than written out here, so they cannot disagree with the order the root
	// signature was actually assembled in.
	UINT PrimitiveCBVIdx	= 0;	// b0 | Per object
	UINT ViewIdx			= 0;	// b1 | One TangentFrameConstants, set as root constants

private:

	// Depth test disabled, the lines show through whatever is in front of them
	GraphicsPSO		OnTopPSO;
};

#endif // D3D12_TANGENTFRAMEPASS_H_INCLUDED
