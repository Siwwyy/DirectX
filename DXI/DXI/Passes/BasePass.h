//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_BASEPASS_H_INCLUDED
#define D3D12_BASEPASS_H_INCLUDED

#include "PassBase.h"

// What the base pass writes into the albedo target instead of the surface colour.
// Values come from the BASE_PASS_DEBUG_* defines of shaders/SharedTypes.h, so the root
// constant this is cast into cannot mean something different in the shader.
//
// Derivatives come from ddx/ddy, which compare a value against its neighbour inside the
// 2x2 pixel quad the GPU always shades together. They are how the hardware knows how fast
// a value changes across the screen, which is what mip selection is built on.
enum class BasePassDebugMode : UINT
{
	Off					= BASE_PASS_DEBUG_OFF,					// Normal rendering
	UVDensity			= BASE_PASS_DEBUG_UV_DENSITY,			// Magnitude of both uv derivatives, texture packed per pixel
	UVDerivativeDdx		= BASE_PASS_DEBUG_UV_DDX,				// Raw signed ddx(uv), u in red and v in green
	UVDerivativeDdy		= BASE_PASS_DEBUG_UV_DDY,				// Raw signed ddy(uv), u in red and v in green
	MipLevel			= BASE_PASS_DEBUG_MIP_LEVEL,			// Texel density, the mip the hardware would pick from it
	WorldDerivatives	= BASE_PASS_DEBUG_WORLD_DERIVATIVES,	// How fast the world position moves per pixel
	NormalDerivatives	= BASE_PASS_DEBUG_NORMAL_DERIVATIVES,	// Where the shading normal jumps between quads

	Count				= BASE_PASS_DEBUG_COUNT
};

class BasePass : public PassBase
{
	DISABLE_COPY(BasePass)

public:

	// Ctors & DCtor
	BasePass();
	~BasePass() = default;

	// Render
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	// Initialize the pipeline state object (PSO) for the base pass
	void RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	void RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept override;

	// Same shaders and root signature as the regular pipeline, only with culling off.
	// Both variants exist up front because the cull mode is baked into the PSO.
	_NODISCARD FORCEINLINE DXPipelineState* GetTwoSidedPipelineState() const noexcept
	{
		return TwoSidedPSO.PipelineState.Get();
	}

	// Pipeline a material should be drawn with
	_NODISCARD DXPipelineState* GetPipelineStateFor(const bool bTwoSided) const noexcept
	{
		return bTwoSided ? GetTwoSidedPipelineState() : GetPipelineState();
	}

	// Root parameter indices, the caller binds them before Render
	// Assigned by the builder in Init rather than written out here, so they cannot
	// disagree with the order the root signature was actually assembled in.
	UINT PrimitiveCBVIdx	= 0;	// b0 | Per object
	UINT MaterialTableIdx	= 0;	// Descriptor table covering every material slot
	UINT DebugModeIdx		= 0;	// b1 | One BasePassDebugMode, set as a root constant

private:

	// Culling disabled, for materials that keep their back faces
	GraphicsPSO		TwoSidedPSO;
};

#endif // D3D12_BASEPASS_H_INCLUDED