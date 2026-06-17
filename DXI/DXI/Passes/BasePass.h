//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_BASEPASS_H_INCLUDED
#define D3D12_BASEPASS_H_INCLUDED

#include "PassBase.h"

class BasePass : public PassBase
{
	DISABLE_COPY(BasePass)

public:

	BasePass();
	~BasePass() = default;

	// Render
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;
	void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;

private:
};

#endif // D3D12_BASEPASS_H_INCLUDED