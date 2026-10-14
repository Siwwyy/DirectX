//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_BASEPASS_H_INCLUDED
#define D3D12_BASEPASS_H_INCLUDED

#include "PassBase.h"

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

private:

	// Culling disabled, for materials that keep their back faces
	GraphicsPSO		TwoSidedPSO;
};

#endif // D3D12_BASEPASS_H_INCLUDED