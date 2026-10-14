//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_AREALIGHT_H_INCLUDED
#define D3D12_AREALIGHT_H_INCLUDED

#include "LightBase.h"

// Light emitting from a rectangular surface, like a softbox or a window.
// Position is the centre of the rectangle and Direction is its normal.
class AreaLight : public LightBase
{
	DISABLE_COPY(AreaLight)

public:

	// Ctors & DCtor
	AreaLight();
	AreaLight(std::wstring Name);
	~AreaLight() = default;

	// LightBase interface
	_NODISCARD LightType GetLightType() const noexcept override
	{
		return LightType::Area;
	}
	_NODISCARD LightConstants GetLightConstants() const noexcept override;

	// Setters
	void SetPosition(const XMFLOAT3 Position) noexcept
	{
		this->Position = Position;
	}
	void SetDirection(const XMFLOAT3 Direction) noexcept;
	void SetSize(const XMFLOAT2 Size) noexcept;
	void SetRange(const float Range) noexcept;

	// Getters
	_NODISCARD inline const auto& GetPosition() const noexcept
	{
		return Position;
	}
	_NODISCARD inline const auto& GetDirection() const noexcept
	{
		return Direction;
	}
	_NODISCARD inline const auto& GetSize() const noexcept
	{
		return Size;
	}
	_NODISCARD inline auto GetRange() const noexcept
	{
		return Range;
	}

private:

	// World space centre of the emitting rectangle
	XMFLOAT3	Position	= XMFLOAT3(0.f, 0.f, 0.f);

	// Normal of the emitting rectangle, kept normalized
	XMFLOAT3	Direction	= XMFLOAT3(0.f, -1.f, 0.f);

	// Width and height of the emitting rectangle
	XMFLOAT2	Size		= XMFLOAT2(1.f, 1.f);

	// Distance at which the light stops contributing
	float		Range		= 10.f;
};

#endif // D3D12_AREALIGHT_H_INCLUDED
