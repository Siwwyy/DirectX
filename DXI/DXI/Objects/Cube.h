//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_CUBE_H_INCLUDED
#define D3D12_CUBE_H_INCLUDED

#include "Primitive.h"

class Cube : public Primitive
{
	DISABLE_COPY(Cube)

public:

	Cube();
	Cube(std::wstring Name);
	~Cube() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList);

private:

};


#endif // D3D12_CUBE_H_INCLUDED