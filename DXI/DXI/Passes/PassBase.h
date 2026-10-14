//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_PASSBASE_H_INCLUDED
#define D3D12_PASSBASE_H_INCLUDED

#include "../pch.h"
#include "../D3D12ShaderCompiler.h"
#include "../Objects/Primitive.h"
#include "../PSO.h"


class PassBase
{
	DISABLE_COPY(PassBase)

public:

	// Ctors & DCtor
	PassBase() = default;
	~PassBase() = default;

	// Init
	virtual HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept = 0; // make this class abstract

	// Render
	virtual void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept = 0; // make this class abstract

	// Pipeline of the pass, the caller binds these before feeding it any work
	_NODISCARD FORCEINLINE DXPipelineState* GetPipelineState() const noexcept
	{
		return PSO.PipelineState.Get();
	}
	_NODISCARD FORCEINLINE ID3D12RootSignature* GetRootSignature() const noexcept
	{
		return PSO.RootSignature.Get();
	}


protected:

	// Pipeline state
	GraphicsPSO		PSO;
};


#endif // D3D12_PASSBASE_H_INCLUDED