//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_PLANE_H_INCLUDED
#define D3D12_PLANE_H_INCLUDED

#include "Primitive.h"

class Plane : public Primitive
{
	DISABLE_COPY(Plane)

public:

	Plane();
	Plane(std::wstring Name);
	~Plane() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList);

private:
};

#endif // D3D12_PLANE_H_INCLUDED