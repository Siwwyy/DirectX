//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SELECTIONPASS_H_INCLUDED
#define D3D12_SELECTIONPASS_H_INCLUDED

#include "PassBase.h"

// SelectionConstants is defined in shaders/SharedTypes.h. Root constants: 64 bytes is 16 of the
// 64 DWORDs a root signature gets.
constexpr UINT SelectionConstantCount = sizeof(SelectionConstants) / sizeof(UINT32);

// How the selected object is marked, picked in the debug panel. The pass's own input: the panel
// only edits it and the renderer hands it on.
struct SelectionSettings
{
	static constexpr float	MIN_TINT_STRENGTH	= 0.0f;
	static constexpr float	MAX_TINT_STRENGTH	= 1.0f;
	static constexpr float	MIN_BOX_THICKNESS	= 1.0f;
	static constexpr float	MAX_BOX_THICKNESS	= 8.0f;

	bool			bEnabled		= true;		// Master switch, off leaves the selection unmarked

	// The surface itself, drawn again with a colour laid over it. Works on any mesh, flat ones
	// and open ones included, where an outline around the silhouette would not.
	bool			bShowTint		= true;
	float			TintStrength	= 0.35f;

	// The box around the object, turned and scaled with it. Visible through the surface, which
	// keeps an object that is half behind another one findable.
	bool			bShowBox		= true;
	float			BoxThickness	= 2.0f;

	// Orange, which no material of the scene is, and which the gizmo's red, green and blue are not
	DirectX::XMFLOAT3	Color		= { 1.00f, 0.55f, 0.10f };
};

// Marks the selected object: its surface tinted, and a box drawn round it.
//
// An overlay like the tangent frame and the origin axes: it runs after the light pass, straight
// into the back buffer, and tests against the depth the base pass wrote. The tint is depth tested
// so it only lands on what is visible, the box is not so it can always be seen.
//
// Two pipelines on one root signature, since a change of shader is a change of pipeline.
class SelectionPass : public PassBase
{
	DISABLE_COPY(SelectionPass)

public:

	// Ctors & DCtor
	SelectionPass();
	~SelectionPass() = default;

	/**
	 * Init
	 */
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	/**
	 * Render | The tint: draws the primitive's own mesh again with the fill pipeline. The caller
	 * binds the root signature, that pipeline, the primitive's constants and the view constants first.
	 */
	void RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	void RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept override;

	/**
	 * Render | The box: twelve edges out of SV_VertexID, no vertex buffer. The caller binds the box
	 * pipeline and the same constants first.
	 */
	void RenderBox(DXGraphicsCommandList * const CommandList) noexcept;

	// Pipeline of the box. The tint's is GetPipelineState().
	_NODISCARD FORCEINLINE DXPipelineState* GetBoxPipelineState() const noexcept
	{
		return BoxPSO.PipelineState.Get();
	}

	// Root parameter indices, assigned by the builder in Init so they cannot disagree with the
	// order the root signature was assembled in
	UINT PrimitiveCBVIdx	= 0;	// b0 | Per object
	UINT ViewIdx			= 0;	// b1 | One SelectionConstants, set as root constants

private:

	// The edges of the box, a different shader and no input layout, otherwise the tint's twin
	GraphicsPSO		BoxPSO;
};

#endif // D3D12_SELECTIONPASS_H_INCLUDED
