//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "DirectionalLight.h"

DirectionalLight::DirectionalLight()
	: LightBase(L"Directional Light")
{
}

DirectionalLight::DirectionalLight(std::wstring Name)
	: LightBase(std::move(Name))
{
}

void DirectionalLight::SetDirection(const XMFLOAT3 Direction) noexcept
{
	// Shading expects a unit vector, normalize once here instead of per pixel
	XMStoreFloat3(&this->Direction, XMVector3Normalize(XMLoadFloat3(&Direction)));
}

void DirectionalLight::SetShadowBounds(const XMFLOAT3 Target, const float Extent, const float Distance) noexcept
{
	DXASSERT(Extent > 0.f, "Shadow box extent must be greater than zero");
	DXASSERT(Distance > 0.f, "Shadow camera distance must be greater than zero");

	ShadowTarget	= Target;
	ShadowExtent	= Extent;
	ShadowDistance	= Distance;
}

XMMATRIX DirectionalLight::GetShadowViewProj() const noexcept
{
	const auto LightDir	= XMVector3Normalize(XMLoadFloat3(&Direction));
	const auto Target	= XMLoadFloat3(&ShadowTarget);

	// Walk back against the direction to find where the light "sits"
	const auto Eye		= XMVectorSubtract(Target, XMVectorScale(LightDir, ShadowDistance));

	// Any up vector works as long as it is not parallel to the direction
	const auto Up		= fabsf(XMVectorGetY(LightDir)) > 0.99f
							? XMVectorSet(0.f, 0.f, 1.f, 0.f)
							: XMVectorSet(0.f, 1.f, 0.f, 0.f);

	const auto View		= XMMatrixLookAtLH(Eye, Target, Up);

	// Parallel rays, so the shadow camera is orthographic. The far plane has to reach
	// past the target, otherwise geometry behind it would be clipped out of the map.
	const auto Proj		= XMMatrixOrthographicLH(ShadowExtent, ShadowExtent, 0.1f, ShadowDistance * 2.f);

	return View * Proj;
}

LightConstants DirectionalLight::GetLightConstants() const noexcept
{
	LightConstants Constants	= MakeBaseConstants();
	Constants.Direction			= Direction;
	return Constants;
}
