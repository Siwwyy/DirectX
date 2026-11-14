//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SPOTLIGHT_H_INCLUDED
#define D3D12_SPOTLIGHT_H_INCLUDED

#include "LightBase.h"

// A point light restricted to a cone, like a torch or a stage lamp. Position is the tip of
// the cone and Direction is the axis it points down. Falls off with distance the way a point
// light does, then again across the cone edge, between the inner and the outer angle.
class SpotLight : public LightBase
{
	DISABLE_COPY(SpotLight)

public:

	// Ctors & DCtor
	SpotLight();
	SpotLight(std::wstring Name);
	~SpotLight() = default;

	// LightBase interface
	_NODISCARD LightType GetLightType() const noexcept override
	{
		return LightType::Spot;
	}
	_NODISCARD Light GetLightConstants() const noexcept override;

	// Setters
	void SetPosition(const XMFLOAT3 Position) noexcept
	{
		this->Position = Position;
	}
	void SetDirection(const XMFLOAT3 Direction) noexcept;
	void SetRange(const float Range) noexcept;
	void SetSourceRadius(const float SourceRadius) noexcept;

	/**
	 * Width of the cone, as half angles in degrees measured from the axis.
	 *
	 * Inner is where the falloff starts and outer is where it reaches nothing, so the gap
	 * between them is the softness of the edge. Equal values give a hard edged circle.
	 *
	 * @param InnerDegrees Full brightness within this angle, clamped to at most OuterDegrees
	 * @param OuterDegrees Edge of the cone, nothing is lit beyond it
	 */
	void SetConeAngles(const float InnerDegrees, const float OuterDegrees) noexcept;

	// Getters
	_NODISCARD inline const auto& GetPosition() const noexcept
	{
		return Position;
	}
	_NODISCARD inline const auto& GetDirection() const noexcept
	{
		return Direction;
	}
	_NODISCARD inline auto GetRange() const noexcept
	{
		return Range;
	}
	_NODISCARD inline auto GetSourceRadius() const noexcept
	{
		return SourceRadius;
	}
	_NODISCARD inline auto GetInnerAngle() const noexcept
	{
		return InnerAngleDegrees;
	}
	_NODISCARD inline auto GetOuterAngle() const noexcept
	{
		return OuterAngleDegrees;
	}

	/**
	 * Shadow camera at the tip of the cone, looking down the axis. One view is enough,
	 * unlike a point light, because a spot only lights what is in front of it.
	 *
	 * @return World to the light's clip space, for its slice of the depth atlas
	 */
	_NODISCARD XMMATRIX GetShadowViewProj() const noexcept;

private:

	// World space tip of the cone
	XMFLOAT3	Position			= XMFLOAT3(0.f, 0.f, 0.f);

	// Axis the cone points down, kept normalized
	XMFLOAT3	Direction			= XMFLOAT3(0.f, -1.f, 0.f);

	// Distance at which the light stops contributing
	float		Range				= 10.f;

	// How big the emitter is, which keeps the falloff finite at the tip
	float		SourceRadius		= 0.1f;

	// Half angles from the axis. Inner is never allowed past outer, see SetConeAngles
	float		InnerAngleDegrees	= 20.f;
	float		OuterAngleDegrees	= 30.f;
};

#endif // D3D12_SPOTLIGHT_H_INCLUDED
