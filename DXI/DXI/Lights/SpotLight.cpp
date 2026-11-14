//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "SpotLight.h"

// Widest cone the light is allowed to open to. A half angle of 90 degrees is already a
// hemisphere, and the shadow projection below needs the full angle to stay under 180
static constexpr float MAX_CONE_HALF_ANGLE = 89.f;

SpotLight::SpotLight()
	: LightBase(L"Spot Light")
{
}

SpotLight::SpotLight(std::wstring Name)
	: LightBase(std::move(Name))
{
}

void SpotLight::SetDirection(const XMFLOAT3 Direction) noexcept
{
	// Shading expects a unit vector, normalize once here instead of per pixel
	XMStoreFloat3(&this->Direction, XMVector3Normalize(XMLoadFloat3(&Direction)));
}

void SpotLight::SetRange(const float Range) noexcept
{
	DXASSERT(Range > 0.f, "Spot light range must be greater than zero, the shader divides by it");
	this->Range = Range;
}

void SpotLight::SetSourceRadius(const float SourceRadius) noexcept
{
	DXASSERT(SourceRadius >= 0.f, "Spot light source radius can not be negative");
	this->SourceRadius = SourceRadius;
}

void SpotLight::SetConeAngles(const float InnerDegrees, const float OuterDegrees) noexcept
{
	DXASSERT(InnerDegrees >= 0.f && OuterDegrees > 0.f, "Cone half angles must be positive");

	OuterAngleDegrees = (OuterDegrees < MAX_CONE_HALF_ANGLE) ? OuterDegrees : MAX_CONE_HALF_ANGLE;

	// An inner wider than the outer would invert the falloff, so it is clamped rather than
	// asserted: dragging the two in the panel would otherwise be order dependent
	InnerAngleDegrees = (InnerDegrees < OuterAngleDegrees) ? InnerDegrees : OuterAngleDegrees;
}

XMMATRIX SpotLight::GetShadowViewProj() const noexcept
{
	const auto ViewPosition	= XMLoadFloat3(&Position);
	const auto Forward		= XMVector3Normalize(XMLoadFloat3(&Direction));

	// Any up vector works as long as it is not parallel to the axis
	const auto Up			= fabsf(XMVectorGetY(Forward)) > 0.99f
								? XMVectorSet(0.f, 0.f, 1.f, 0.f)
								: XMVectorSet(0.f, 1.f, 0.f, 0.f);

	const auto View			= XMMatrixLookToLH(ViewPosition, Forward, Up);

	// The cone is what the light reaches, so the frustum is sized to it rather than fixed.
	// Twice the half angle is the full field of view, and the square map wants an aspect
	// of one. A little wider than the cone so the edge of the falloff is not clipped.
	const float FovRadians	= 2.f * XMConvertToRadians(OuterAngleDegrees) * 1.1f;
	const float ClampedFov	= (FovRadians < XM_PI * 0.98f) ? FovRadians : XM_PI * 0.98f;

	const auto Proj			= XMMatrixPerspectiveFovLH(ClampedFov, 1.f, 0.05f, Range);

	return View * Proj;
}

Light SpotLight::GetLightConstants() const noexcept
{
	Light Constants	= MakeBaseConstants();
	Constants.Position		= XMFLOAT4(Position.x, Position.y, Position.z, 1.f);
	Constants.Direction		= XMFLOAT4(Direction.x, Direction.y, Direction.z, 0.f);
	Constants.Range			= Range;
	Constants.SourceRadius	= SourceRadius;

	// Cosines rather than angles, so the shader compares against dot() without a trig call
	Constants.SpotCosInner	= cosf(XMConvertToRadians(InnerAngleDegrees));
	Constants.SpotCosOuter	= cosf(XMConvertToRadians(OuterAngleDegrees));
	return Constants;
}
