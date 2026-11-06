//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_LIGHTBASE_H_INCLUDED
#define D3D12_LIGHTBASE_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIMath.h"

// MAX_LIGHTS, NO_SHADOW_CASTER, LIGHT_TYPE_* and the Light struct itself
#include "SharedTypes.h"

// Kind of the light. Values come from the shared header so the Type switch inside the
// shaders cannot drift from this.
enum class LightType : UINT
{
	Directional	= LIGHT_TYPE_DIRECTIONAL,
	Point		= LIGHT_TYPE_POINT,
	Area		= LIGHT_TYPE_AREA,

	Count
};

// Light name, for debug purposes
_NODISCARD const wchar_t* ToString(const LightType Type);


// Light and LightsConstantBuffer are defined in shaders/SharedTypes.h, so there is one of
// each rather than a CPU copy and an HLSL copy. Mind the field alignment note there before
// adding to or reordering either of them.
constexpr auto LightsConstantBufferSize			= sizeof(LightsConstantBuffer);
constexpr auto LightsConstantBufferAlignment	= alignof(LightsConstantBuffer);


// Base of every light type. Holds what all of them share, the derived classes add
// their own properties and pack themselves into the GPU layout.
class LightBase
{
	DISABLE_COPY(LightBase)

public:

	// Ctors & DCtor
	LightBase()	= default;
	LightBase(std::wstring Name);
	virtual ~LightBase() = default;

	// Kind of this light
	_NODISCARD virtual LightType GetLightType() const noexcept = 0;		// make this class abstract

	// Packs this light into the layout the shaders read
	_NODISCARD virtual Light GetLightConstants() const noexcept = 0;

	// Setters
	void SetColor(const XMFLOAT3 Color) noexcept
	{
		this->Color = Color;
	}
	void SetIntensity(const float Intensity) noexcept
	{
		this->Intensity = Intensity;
	}

	// Getters
	_NODISCARD inline const auto& GetColor() const noexcept
	{
		return Color;
	}
	_NODISCARD inline auto GetIntensity() const noexcept
	{
		return Intensity;
	}
	_NODISCARD inline const auto& GetName() const noexcept
	{
		return Name;
	}

protected:

	// Fills everything the base knows about, the derived class completes the rest
	_NODISCARD Light MakeBaseConstants() const noexcept;

	// Linear colour of the light
	XMFLOAT3		Color		= XMFLOAT3(1.f, 1.f, 1.f);

	// Brightness multiplier of the colour
	float			Intensity	= 1.f;

	// Name of light (for debug purposes)
	std::wstring	Name		= L"Light";
};

#endif // D3D12_LIGHTBASE_H_INCLUDED
