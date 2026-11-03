//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_DIRECTIONALLIGHT_H_INCLUDED
#define D3D12_DIRECTIONALLIGHT_H_INCLUDED

#include "LightBase.h"

// Light coming from infinitely far away, like the sun.
// Has no position, every pixel of the scene is lit from the same direction.
class DirectionalLight : public LightBase
{
	DISABLE_COPY(DirectionalLight)

public:

	// Ctors & DCtor
	DirectionalLight();
	DirectionalLight(std::wstring Name);
	~DirectionalLight() = default;

	// LightBase interface
	_NODISCARD LightType GetLightType() const noexcept override
	{
		return LightType::Directional;
	}
	_NODISCARD LightConstants GetLightConstants() const noexcept override;

	// Setters
	void SetDirection(const XMFLOAT3 Direction) noexcept;

	// Shadow camera | The box the shadow map covers, centred on Target
	void SetShadowBounds(const XMFLOAT3 Target, const float Extent, const float Distance) noexcept;

	// Getters
	_NODISCARD inline const auto& GetDirection() const noexcept
	{
		return Direction;
	}

	// View projection the shadow pass renders with. A directional light has no position,
	// so the shadow camera is placed Distance away along the direction, aimed at Target.
	_NODISCARD XMMATRIX GetShadowViewProj() const noexcept;

private:

	// Direction the light travels towards, kept normalized
	XMFLOAT3	Direction		= XMFLOAT3(0.f, -1.f, 0.f);

	// Centre of the area the shadow map covers, usually the middle of the scene
	XMFLOAT3	ShadowTarget	= XMFLOAT3(0.f, 0.f, 0.f);

	// Width and height of the orthographic shadow box, in world units
	float		ShadowExtent	= 20.f;

	// How far back along the direction the shadow camera sits
	float		ShadowDistance	= 20.f;
};

#endif // D3D12_DIRECTIONALLIGHT_H_INCLUDED
