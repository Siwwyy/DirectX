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

	/**
	 * Init
	 */
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList);

	// Four vertices, but stated rather than left to the count: the shapes that are always shown
	// are a decision about the shape, not an effect of how many vertices it happens to have
	_NODISCARD bool AlwaysShowsAllFrameVertices() const noexcept override
	{
		return true;
	}

private:
};

#endif // D3D12_PLANE_H_INCLUDED
