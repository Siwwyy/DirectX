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

	/**
	 * Creates the depth texture, its depth stencil view and nothing else. Safe to call
	 * again to resize, the old resource is released as the ComPtr is reassigned.
	 */
	void Init(const DXIDevice& Device, const UINT Width, const UINT Height);

	/**
	 * Format the passes build their PSOs against, and the one the depth stencil view uses.
	 *
	 * Three formats exist because the resource is typeless, so it can be both rendered
	 * through and sampled: a depth format cannot be given to a shader resource view, and a
	 * plain float format cannot be given to a depth stencil view.
	 */
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	}

	/**
	 * Format the resource itself is created with, typeless so both views below are legal.
	 */
	_NODISCARD static DXGI_FORMAT GetResourceFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
	}

	/**
	 * Format LightPass samples the same memory through.
	 */
	_NODISCARD static DXGI_FORMAT GetSrvFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	}

	/**
	 * Resets the depth to the far plane. Once per frame, before anything draws.
	 *
	 * Requires the target in the depth write state, so TransitionToDepthWrite has to have
	 * run first. Asserts rather than silently clearing a readable target.
	 */
	void Clear(DXGraphicsCommandList * CommandList) const;

	/**
	 * View the passes render against.
	 */
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

	/**
	 * Writes the shader resource view, so LightPass can sample depth through the same heap
	 * it reads the G-buffer from.
	 *
	 * @param Handle    Where the view is written. The caller owns the heap, and has to
	 *                  rewrite this whenever Init runs again, or it outlives its resource.
	 */
	void CreateShaderResourceView(const DXIDevice& Device, const D3D12_CPU_DESCRIPTOR_HANDLE Handle) const;

	/**
	 * Barriers. BasePass renders depth, LightPass samples it.
	 *
	 * The current state is tracked the way GBuffer tracks its targets, so either call is
	 * free when the target already sits in that state.
	 */
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
