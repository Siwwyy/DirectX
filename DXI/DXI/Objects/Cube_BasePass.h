//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_CUBE_BASEPASS_H_INCLUDED
#define D3D12_CUBE_BASEPASS_H_INCLUDED

#include "Primitive.h"

class Cube_BasePass : public Primitive
{
	DISABLE_COPY(Cube_BasePass)

public:

	Cube_BasePass();
	~Cube_BasePass() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList);

private:

};

#endif // D3D12_CUBE_BASEPASS_H_INCLUDED