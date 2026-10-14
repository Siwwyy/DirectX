//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_LIGHTBASE_H_INCLUDED
#define D3D12_LIGHTBASE_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "../D3D12Math.h"

// Kind of the light, has to match the Type switch inside the shaders
enum class LightType : UINT
{
	Directional = 0,
	Point,
	Area,

	Count
};

// How many lights fit in the light constant buffer.
// Keep this in sync with MAX_LIGHTS inside shaders/Lighting.hlsl
constexpr UINT MAX_LIGHTS = 4;

// Value of ShadowLightIdx when no light casts a shadow this frame
constexpr INT NO_SHADOW_CASTER = -1;

// Light name, for debug purposes
_NODISCARD const wchar_t* ToString(const LightType Type);


// A single light as the shaders see it. Not every field matters for every type,
// e.g., Range is point only and Size is area only, the shader branches on Type.
// Mirrors the Light struct of shaders/Common.hlsl, mind the 16 bytes rows when editing.
struct LightConstants
{
	DirectX::XMFLOAT3	Position;		// World space position | Point, Area
	float				Intensity;		// Brightness multiplier

	DirectX::XMFLOAT3	Direction;		// Direction the light points at | Directional, Area normal
	float				Range;			// Distance the light reaches | Point

	DirectX::XMFLOAT3	Color;			// Linear colour
	UINT				Type;			// LightType

	DirectX::XMFLOAT2	Size;			// Width and height of the emitting surface | Area
	DirectX::XMFLOAT2	Padding;		// Keeps the struct at a 16 bytes boundary
};

static_assert((sizeof(LightConstants) % 16) == 0, "LightConstants must be aligned to 16-bytes rows to match the HLSL packing");


// Constant buffer holding every light of the scene.
// Only one light casts a shadow for now, a point light would need a cube map and an
// area light more than that, so the shadow matrix is stored once instead of per light.
struct alignas(256) LightsConstantBuffer
{
	LightConstants		Lights[MAX_LIGHTS];	// All lights of the scene
	UINT				NumLights;			// How many entries of Lights are filled
	INT					ShadowLightIdx;		// Index into Lights of the shadow caster, -1 when nothing casts
	DirectX::XMFLOAT2	Padding;			// Keeps the matrix below on a 16 bytes boundary
	DirectX::XMFLOAT4X4 ShadowViewProj;		// World to the shadow caster's clip space, already transposed for HLSL
};

static_assert((sizeof(LightsConstantBuffer) % 256) == 0, "Lights Constant Buffer size must be aligned to 256-bytes boudary");
static_assert(offsetof(LightsConstantBuffer, ShadowViewProj) % 16 == 0, "HLSL starts a matrix on a 16-bytes row, the padding above it must keep that");
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
	_NODISCARD virtual LightConstants GetLightConstants() const noexcept = 0;

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
	_NODISCARD LightConstants MakeBaseConstants() const noexcept;

	// Linear colour of the light
	XMFLOAT3		Color		= XMFLOAT3(1.f, 1.f, 1.f);

	// Brightness multiplier of the colour
	float			Intensity	= 1.f;

	// Name of light (for debug purposes)
	std::wstring	Name		= L"Light";
};

#endif // D3D12_LIGHTBASE_H_INCLUDED
