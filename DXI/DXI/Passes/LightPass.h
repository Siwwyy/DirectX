//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_LIGHTPASS_H_INCLUDED
#define D3D12_LIGHTPASS_H_INCLUDED

#include "PassBase.h"

// Shades the G-buffer BasePass wrote, using every light placed in the scene.
// Draws one fullscreen triangle, so it takes no geometry of its own.
class LightPass : public PassBase
{
	DISABLE_COPY(LightPass)

public:

	// Ctors & DCtor
	LightPass();
	~LightPass() = default;
	
	// Render | LightPass is fullscreen, there is no per primitive work, use RenderFullscreen instead
	void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override; 

	// Draws the fullscreen triangle that shades the whole G-buffer
	void RenderFullscreen(DXGraphicsCommandList * const CommandList) noexcept;

	// Root parameter indices, the caller binds them before RenderFullscreen
	enum RootParamIdx : UINT
	{
		GBufferTableIdx = 0,	// Descriptor table covering every G-buffer target plus the shadow map
		CameraCBVIdx,			// b1 | Per camera
		LightsCBVIdx			// b2 | Per lights of the scene
	};

private:


};

#endif // D3D12_LIGHTPASS_H_INCLUDED
