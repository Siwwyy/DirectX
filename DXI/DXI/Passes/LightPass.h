//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_LIGHTPASS_H_INCLUDED
#define D3D12_LIGHTPASS_H_INCLUDED

#include "PassBase.h"
#include "GBuffer.h"
#include "ShadowAtlas.h"

// Shades the G-buffer BasePass wrote, using every light placed in the scene.
// Draws one fullscreen triangle, so it takes no geometry of its own.
class LightPass : public PassBase
{
	DISABLE_COPY(LightPass)

public:

	// Ctors & DCtor
	LightPass();
	~LightPass() = default;
	
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override; 

	// Render | LightPass is fullscreen, there is no per primitive work, use RenderFullscreen instead
	void RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;

	// Draws the fullscreen triangle that shades the whole G-buffer
	void RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept override;

	// Radiance and irradiance, the two sky cubes the reflections were gathered from. Zero
	// while the sky is out; putting it back to 2 is the first step of restoring it.
	static constexpr UINT SKY_SRV_COUNT = 0;

	// Everything the pass samples, in t0 to t7 order: the G-buffer targets, the shadow
	// atlas, then the sky cubes. The root signature range below and the heap the caller
	// fills have to be exactly this long, so both take the number from here.
	_NODISCARD static constexpr UINT GetSrvCount() noexcept
	{
		return GBufferTargetCount + ShadowAtlas::GetSrvCount() + SKY_SRV_COUNT;
	}

	// Root parameter indices, the caller binds them before RenderFullscreen. Assigned by
	// the builder in Init rather than written out here, so they cannot disagree with the
	// order the root signature was actually assembled in.
	UINT GBufferTableIdx	= 0;	// Descriptor table covering the G-buffer, shadow atlas and sky cubes
	UINT CameraCBVIdx		= 0;	// b1 | Per camera
	UINT LightsCBVIdx		= 0;	// b2 | Per lights of the scene

private:


};

#endif // D3D12_LIGHTPASS_H_INCLUDED
