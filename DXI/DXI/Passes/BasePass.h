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

private:


};

#endif // D3D12_BASEPASS_H_INCLUDED