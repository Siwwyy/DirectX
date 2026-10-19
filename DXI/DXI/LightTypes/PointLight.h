//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_POINTLIGHT_H_INCLUDED
#define D3D12_POINTLIGHT_H_INCLUDED

#include "LightBase.h"

// Light emitting from a single point in every direction, like a bulb.
// Fades out with the distance and stops contributing past its range.
class PointLight : public LightBase
{
	DISABLE_COPY(PointLight)

public:

	// Ctors & DCtor
	PointLight();
	PointLight(std::wstring Name);
	~PointLight() = default;

	// LightBase interface
	_NODISCARD LightType GetLightType() const noexcept override
	{
		return LightType::Point;
	}
	_NODISCARD LightConstants GetLightConstants() const noexcept override;

	// Setters
	void SetPosition(const XMFLOAT3 Position) noexcept
	{
		this->Position = Position;
	}
	void SetRange(const float Range) noexcept;

	// Getters
	_NODISCARD inline const auto& GetPosition() const noexcept
	{
		return Position;
	}
	_NODISCARD inline auto GetRange() const noexcept
	{
		return Range;
	}

	// A point light shines everywhere, so its shadow needs all six directions.
	// FaceIdx follows the D3D cube order +X -X +Y -Y +Z -Z, and each face is a 90 degree
	// perspective, which is exactly what tiles a cube without gaps or overlap.
	_NODISCARD XMMATRIX GetShadowViewProj(const UINT FaceIdx) const noexcept;

	// Near plane the shadow faces are rendered with, the lighting needs it to match
	_NODISCARD static constexpr float GetShadowNearPlane() noexcept
	{
		return 0.05f;
	}

private:

	// World space position of the bulb
	XMFLOAT3	Position	= XMFLOAT3(0.f, 0.f, 0.f);

	// Distance at which the light stops contributing
	float		Range		= 10.f;
};

#endif // D3D12_POINTLIGHT_H_INCLUDED
