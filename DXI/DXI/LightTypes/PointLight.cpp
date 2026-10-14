//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "PointLight.h"

PointLight::PointLight()
	: LightBase(L"Point Light")
{
}

PointLight::PointLight(std::wstring Name)
	: LightBase(std::move(Name))
{
}

void PointLight::SetRange(const float Range) noexcept
{
	DXASSERT(Range > 0.f, "Point light range must be greater than zero, the shader divides by it");
	this->Range = Range;
}

LightConstants PointLight::GetLightConstants() const noexcept
{
	LightConstants Constants	= MakeBaseConstants();
	Constants.Position			= Position;
	Constants.Range				= Range;
	return Constants;
}
