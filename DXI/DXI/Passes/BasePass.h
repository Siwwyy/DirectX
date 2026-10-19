//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_BASEPASS_H_INCLUDED
#define D3D12_BASEPASS_H_INCLUDED

#include "PassBase.h"

// What the base pass writes into the albedo target instead of the surface colour.
// Keep in sync with the BASE_PASS_DEBUG_* defines of shaders/Common.hlsl.
//
// Derivatives come from ddx/ddy, which compare a value against its neighbour inside the
// 2x2 pixel quad the GPU always shades together. They are how the hardware knows how fast
// a value changes across the screen, which is what mip selection is built on.
enum class BasePassDebugMode : UINT
{
	Off = 0,			// Normal rendering
	UVDensity,			// Magnitude of both uv derivatives, texture packed per pixel
	UVDerivativeDdx,	// Raw signed ddx(uv), u in red and v in green
	UVDerivativeDdy,	// Raw signed ddy(uv), u in red and v in green
	MipLevel,			// Texel density, the mip the hardware would pick from it
	WorldDerivatives,	// How fast the world position moves per pixel
	NormalDerivatives,	// Where the shading normal jumps between quads

	Count
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
	void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;

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
	enum RootParamIdx : UINT
	{
		PrimitiveCBVIdx = 0,	// b0 | Per object
		MaterialTableIdx,		// Descriptor table covering every material slot
		DebugModeIdx			// b1 | One BasePassDebugMode, set as a root constant
	};

private:

	// Culling disabled, for materials that keep their back faces
	GraphicsPSO		TwoSidedPSO;
};

#endif // D3D12_BASEPASS_H_INCLUDED