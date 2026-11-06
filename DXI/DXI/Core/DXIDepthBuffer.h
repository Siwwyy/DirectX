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

#endif // DXI_DEPTHBUFFER_H_INCLUDED
