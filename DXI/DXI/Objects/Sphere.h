//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SPHERE_H_INCLUDED
#define D3D12_SPHERE_H_INCLUDED

#include "Primitive.h"

// A UV sphere, built at Init rather than from a static array.
//
// Cube and Plane can size their buffers at compile time because their vertices are a fixed
// literal list. A sphere cannot: the density is only known once it has been asked for, so
// the geometry is generated the way Model loads one, straight into vectors.
//
// Radius is 0.5, matching the unit cube, so the two look the same size side by side.
class Sphere : public Primitive
{
	DISABLE_COPY(Sphere)

public:

	// Below three latitude bands there is no recognisable sphere left to build
	static constexpr UINT MIN_DENSITY		= 3;
	static constexpr UINT MAX_DENSITY		= 128;
	static constexpr UINT DEFAULT_DENSITY	= 24;

	Sphere() = default;
	Sphere(std::wstring Name, const UINT Density = DEFAULT_DENSITY);
	~Sphere() override = default;

	// Generates the mesh for the current density, then uploads it
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList) override;

	// Latitude bands. Longitude segments come out at twice this, which keeps the
	// triangles roughly equilateral: longitude wraps a full 360 degrees while latitude
	// only spans 180.
	//
	// Has to be set before Init. Changing it afterwards does not rebuild the vertex
	// buffer, which would need a command list to upload the new one with.
	void SetDensity(const UINT InDensity) noexcept;

	_NODISCARD UINT GetDensity() const noexcept
	{
		return Density;
	}

	// Triangles the given density produces. Two per quad, except the rows against each
	// pole which degenerate into a single fan.
	_NODISCARD static UINT GetTriangleCountFor(const UINT Density) noexcept;

private:

	UINT Density = DEFAULT_DENSITY;
};

#endif // D3D12_SPHERE_H_INCLUDED
