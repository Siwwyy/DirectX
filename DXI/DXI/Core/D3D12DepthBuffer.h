//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_DEPTHBUFFER_H_INCLUDED
#define D3D12_DEPTHBUFFER_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "D3D12Device.h"

// Depth target along with its descriptor heap and view.
class D3D12DepthBuffer
{
	DISABLE_COPY_MOVE(D3D12DepthBuffer)

public:

	D3D12DepthBuffer()	= default;
	~D3D12DepthBuffer()	= default;

	// Creates the depth texture and its view
	void Initialize(const D3D12Device& Device, const UINT Width, const UINT Height);

	// Format the depth target is created with, the passes need it for their PSOs
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	}

	// Resets the depth to the far plane, do it once per frame before anything draws
	void Clear(DXGraphicsCommandList * CommandList) const;

	// View the passes render against
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

private:

	// Value Clear() writes, 1 is the far plane of a standard depth range
	static constexpr float DEPTH_CLEAR_VALUE = 1.0f;

	ComPtr<DXResource>			DepthStencil;	// the depth target itself
	ComPtr<DXDescriptorHeap>	DsvHeap;		// holds the single depth stencil view
};

#endif // D3D12_DEPTHBUFFER_H_INCLUDED
