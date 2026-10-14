//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "LightBase.h"

const wchar_t* ToString(const LightType Type)
{
	switch (Type)
	{
	case LightType::Directional:	return L"Directional";
	case LightType::Point:			return L"Point";
	case LightType::Area:			return L"Area";
	default:						return L"Unknown";
	}
}

LightBase::LightBase(std::wstring Name)
	: Name(std::move(Name))
{
}

LightConstants LightBase::MakeBaseConstants() const noexcept
{
	LightConstants Constants	= {};
	Constants.Position			= XMFLOAT3(0.f, 0.f, 0.f);
	Constants.Intensity			= Intensity;
	Constants.Direction			= XMFLOAT3(0.f, -1.f, 0.f);
	Constants.Range				= 0.f;
	Constants.Color				= Color;
	Constants.Type				= static_cast<UINT>(GetLightType());
	Constants.Size				= XMFLOAT2(0.f, 0.f);
	Constants.Padding			= XMFLOAT2(0.f, 0.f);
	return Constants;
}
