//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "LightBase.h"

const wchar_t* ToString(const LightType Type)
{
	switch (Type)
	{
	case LightType::Directional:	return L"Directional";
	case LightType::Point:			return L"Point";
	case LightType::Area:			return L"Area";
	case LightType::Spot:			return L"Spot";
	default:						return L"Unknown";
	}
}

LightBase::LightBase(std::wstring Name)
	: Name(std::move(Name))
{
}

Light LightBase::MakeBaseConstants() const noexcept
{
	Light Constants	= {};

	// w 0 on both, which is already right for a directional light: it has no position, and
	// a direction never takes translation. The types that do have one overwrite it with w 1
	Constants.Position			= XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	Constants.Direction			= XMFLOAT4(0.f, -1.f, 0.f, 0.f);
	Constants.Color				= Color;
	Constants.Type				= static_cast<UINT>(GetLightType());
	Constants.Intensity			= Intensity;
	Constants.Range				= 0.f;
	Constants.SourceRadius		= 0.f;					// only a point light has a size that matters here
	Constants.ShadowSlot		= NO_SHADOW_CASTER;		// the renderer fills this once it hands out atlas slices
	Constants.Size				= XMFLOAT2(0.f, 0.f);
	Constants.SpecularScale		= SpecularScale;
	Constants.TypeParams		= XMFLOAT3(0.f, 0.f, 0.f);
	return Constants;
}
