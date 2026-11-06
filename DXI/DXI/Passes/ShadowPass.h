//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADOWPASS_H_INCLUDED
#define D3D12_SHADOWPASS_H_INCLUDED

#include "PassBase.h"

// ShadowViewConstants is defined in shaders/SharedTypes.h. WorldToLightClip is already
// transposed for HLSL by the time it is written there.
//
// This count is why the struct is not padded to 256 like the CBV backed buffers: it is
// sent as root constants, and at 80 bytes that is 20 of the 64 DWORDs a root signature
// gets. Padding it would spend the entire budget.
constexpr UINT ShadowViewConstantCount = sizeof(ShadowViewConstants) / sizeof(UINT32);


// Renders the scene from a light.
//
// Two pipelines, because the two kinds of light store different things:
//   Depth    | Directional and area lights write depth only, with no pixel shader at all
//   Distance | A point light writes how far the light reached into a colour target, so a
//              lookup later is a single cube sample by direction
class ShadowPass : public PassBase
{
	DISABLE_COPY(ShadowPass)

public:

	// Ctors & DCtor
	ShadowPass();
	~ShadowPass() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	// Render | Draws one primitive into whichever view is currently bound
	void RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	void RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept override;

	// Pipeline that writes distance into a cube face, instead of depth only
	_NODISCARD FORCEINLINE DXPipelineState* GetDistancePipelineState() const noexcept
	{
		return DistancePSO.PipelineState.Get();
	}

	// Root parameter indices, the caller binds them before Render
	// Assigned by the builder in Init rather than written out here, so they cannot
	// disagree with the order the root signature was actually assembled in.
	UINT PrimitiveCBVIdx	= 0;	// b0 | Per object, only LocalToWorld is read
	UINT ShadowViewIdx		= 0;	// b1 | One ShadowViewConstants, set as root constants

private:

	// Writes the distance to the light rather than depth, for the cube faces
	GraphicsPSO		DistancePSO;
};

#endif // D3D12_SHADOWPASS_H_INCLUDED
