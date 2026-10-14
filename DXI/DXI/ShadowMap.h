//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADOWMAP_H_INCLUDED
#define D3D12_SHADOWMAP_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"

// Depth of the scene as seen from a light.
// ShadowPass renders into it and LightPass samples it to decide what is occluded.
class ShadowMap
{
	DISABLE_COPY_MOVE(ShadowMap)

public:

	ShadowMap()		= default;
	~ShadowMap()	= default;

	// Creates the depth target along with its own viewport and descriptor
	void Initialize(DXDevice * Device, const UINT Resolution);

	// Format the depth target is created with, ShadowPass needs it for its PSO
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	}

	// Format the shader samples it through, a depth format can not be read directly
	_NODISCARD static DXGI_FORMAT GetSrvFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	}

	// Writes the shader resource view at the given slot. The caller owns the heap, because
	// LightPass needs this view and the G-buffer in one single shader visible heap.
	void CreateShaderResourceView(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle) const;

	// Barriers | ShadowPass writes the depth, LightPass samples it
	void TransitionToDepthWrite(DXGraphicsCommandList * CommandList);
	void TransitionToShaderResource(DXGraphicsCommandList * CommandList);

	// Binds the depth target with no colour attached and resets it to the far plane
	void BindAsDepthTarget(DXGraphicsCommandList * CommandList) const;
	void Clear(DXGraphicsCommandList * CommandList) const;

	// Getters
	_NODISCARD FORCEINLINE UINT GetResolution() const noexcept
	{
		return Resolution;
	}
	_NODISCARD FORCEINLINE const CD3DX12_VIEWPORT& GetViewPort() const noexcept
	{
		return ViewPort;
	}
	_NODISCARD FORCEINLINE const CD3DX12_RECT& GetScissorRect() const noexcept
	{
		return ScissorRect;
	}

private:

	// Value Clear() writes, 1 is the far plane of a standard depth range
	static constexpr float DEPTH_CLEAR_VALUE = 1.0f;

	ComPtr<DXResource>			DepthTarget;				// the depth of the scene from the light
	ComPtr<DXDescriptorHeap>	DsvHeap;					// holds the single depth stencil view
	UINT						Resolution		= 0;		// square, so one number covers both axes

	// The shadow map is its own render size, so it carries its own viewport
	CD3DX12_VIEWPORT			ViewPort		= CD3DX12_VIEWPORT(0.f, 0.f, 0.f, 0.f);
	CD3DX12_RECT				ScissorRect		= CD3DX12_RECT(0, 0, 0, 0);

	// State the target currently sits in, both transitions are no-ops when already there
	D3D12_RESOURCE_STATES		CurrentState	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};

#endif // D3D12_SHADOWMAP_H_INCLUDED
