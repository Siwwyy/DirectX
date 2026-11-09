//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_DEPTHBUFFER_H_INCLUDED
#define DXI_DEPTHBUFFER_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIDevice.h"

// Depth target along with its descriptor heap and view.
class DXIDepthBuffer
{
	DISABLE_COPY_MOVE(DXIDepthBuffer)

public:

	DXIDepthBuffer()	= default;
	~DXIDepthBuffer()	= default;

	// Creates the depth texture and its view
	void Init(const DXIDevice& Device, const UINT Width, const UINT Height);

	// Three formats, because the resource is typeless so it can be both rendered through
	// and sampled. A depth format cannot be given to a shader resource view, and a plain
	// float format cannot be given to a depth stencil view, so neither can be the format
	// the resource itself is created with.
	//
	// Format the passes need for their PSOs, and the one the depth stencil view uses
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	}

	// Format the resource is created with
	_NODISCARD static DXGI_FORMAT GetResourceFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
	}

	// Format LightPass samples it through
	_NODISCARD static DXGI_FORMAT GetSrvFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	}

	// Resets the depth to the far plane, do it once per frame before anything draws.
	// Needs the target in the depth write state, so call TransitionToDepthWrite first.
	void Clear(DXGraphicsCommandList * CommandList) const;

	// View the passes render against
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

	// Writes the shader resource view into a heap the caller owns, so LightPass can sample
	// depth through the same heap it reads the G-buffer from
	void CreateShaderResourceView(const DXIDevice& Device, const D3D12_CPU_DESCRIPTOR_HANDLE Handle) const;

	// Barriers | BasePass renders depth, LightPass samples it. Tracked the same way GBuffer
	// tracks its targets, so both calls are free when the target is already in that state.
	void TransitionToDepthWrite(DXGraphicsCommandList * CommandList);
	void TransitionToShaderResource(DXGraphicsCommandList * CommandList);

private:

	// Value Clear() writes, 1 is the far plane of a standard depth range
	static constexpr float DEPTH_CLEAR_VALUE = 1.0f;

	ComPtr<DXResource>			DepthStencil;	// the depth target itself
	ComPtr<DXDescriptorHeap>	DsvHeap;		// holds the single depth stencil view

	// State the target currently sits in. Init creates it ready to render through.
	D3D12_RESOURCE_STATES		CurrentState	= D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

#endif // DXI_DEPTHBUFFER_H_INCLUDED
