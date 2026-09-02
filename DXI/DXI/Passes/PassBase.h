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


protected:

	// Pipeline state
	GraphicsPSO		PSO;
};


#endif // D3D12_PASSBASE_H_INCLUDED