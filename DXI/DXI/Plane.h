//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_PLANE_H_INCLUDED
#define D3D12_PLANE_H_INCLUDED

#include "Objects/Primitive.h"

class Plane : public Primitive
{
	DISABLE_COPY(Plane)

public:

	Plane();
	Plane(const DirectX::XMFLOAT4 InitRot,
		  const DirectX::XMFLOAT4 InitPosition,
		  const XMFLOAT4 InitScale);
	~Plane() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList);

private:

};


#endif // D3D12_PLANE_H_INCLUDED