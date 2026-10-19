//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "AreaLight.h"

AreaLight::AreaLight()
	: LightBase(L"Area Light")
{
}

AreaLight::AreaLight(std::wstring Name)
	: LightBase(std::move(Name))
{
}

void AreaLight::SetDirection(const XMFLOAT3 Direction) noexcept
{
	// Shading expects a unit vector, normalize once here instead of per pixel
	XMStoreFloat3(&this->Direction, XMVector3Normalize(XMLoadFloat3(&Direction)));
}

void AreaLight::SetSize(const XMFLOAT2 Size) noexcept
{
	DXASSERT(Size.x > 0.f && Size.y > 0.f, "Area light size must be greater than zero in both axes");
	this->Size = Size;
}

void AreaLight::SetRange(const float Range) noexcept
{
	DXASSERT(Range > 0.f, "Area light range must be greater than zero, the shader divides by it");
	this->Range = Range;
}

XMMATRIX AreaLight::GetShadowViewProj() const noexcept
{
	const auto Eye		= XMLoadFloat3(&Position);
	const auto Forward	= XMVector3Normalize(XMLoadFloat3(&Direction));

	// Any up vector works as long as it is not parallel to the normal
	const auto Up		= fabsf(XMVectorGetY(Forward)) > 0.99f
							? XMVectorSet(0.f, 0.f, 1.f, 0.f)
							: XMVectorSet(0.f, 1.f, 0.f, 0.f);

	const auto View		= XMMatrixLookToLH(Eye, Forward, Up);

	// Wide enough to cover what the rectangle can plausibly light. A narrower cone would
	// give sharper shadows but clip anything off to the side.
	const auto Proj		= XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.05f, Range);

	return View * Proj;
}

LightConstants AreaLight::GetLightConstants() const noexcept
{
	LightConstants Constants	= MakeBaseConstants();
	Constants.Position			= Position;
	Constants.Direction			= Direction;
	Constants.Size				= Size;
	Constants.Range				= Range;
	return Constants;
}
