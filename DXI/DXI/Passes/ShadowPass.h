//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADOWPASS_H_INCLUDED
#define D3D12_SHADOWPASS_H_INCLUDED

#include "PassBase.h"

// Renders the depth of the scene from the shadow casting light.
// Depth only, so it has no pixel shader and no render target at all.
class ShadowPass : public PassBase
{
	DISABLE_COPY(ShadowPass)

public:

	// Ctors & DCtor
	ShadowPass();
	~ShadowPass() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	// Render | Draws one primitive into the shadow map
	void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;

	// Root parameter indices, the caller binds them before Render
	enum RootParamIdx : UINT
	{
		PrimitiveCBVIdx = 0		// b0 | Per object, only LocalToLightClip is read
	};

private:


};

#endif // D3D12_SHADOWPASS_H_INCLUDED
