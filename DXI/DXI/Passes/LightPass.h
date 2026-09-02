//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_LIGHTPASS_H_INCLUDED
#define D3D12_LIGHTPASS_H_INCLUDED

#include "PassBase.h"

class LightPass : public PassBase
{
	DISABLE_COPY(LightPass)

public:

	// Ctors & DCtor
	LightPass();
	~LightPass() = default;
	
	// Render
	void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override; 

private:


};

#endif // D3D12_LIGHTPASS_H_INCLUDED