//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SWAPCHAIN_H_INCLUDED
#define D3D12_SWAPCHAIN_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "D3D12Device.h"

// Swap chain along with its back buffers and their render target views.
// Owns the barriers that flip a back buffer between presenting and being drawn into.
class D3D12SwapChain
{
	DISABLE_COPY_MOVE(D3D12SwapChain)

public:

	D3D12SwapChain()	= default;
	~D3D12SwapChain()	= default;

	// Creates the swap chain for the given window and a render target view per back buffer
	void Initialize(const D3D12Device& Device, const HWND WindowHandle, const UINT Width, const UINT Height, const UINT BackBufferCount);

	// Format every back buffer is created with
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	// Index of the back buffer the next frame goes into
	_NODISCARD UINT GetCurrentBackBufferIdx() const;

	// Render target view of one of the back buffers
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(const UINT BackBufferIdx) const;

	// Barriers | A back buffer is presentable until a pass draws into it
	void TransitionToRenderTarget(DXGraphicsCommandList * CommandList, const UINT BackBufferIdx) const;
	void TransitionToPresent(DXGraphicsCommandList * CommandList, const UINT BackBufferIdx) const;

	// Hands the finished frame over to the compositor.
	// A SyncInterval of 0 presents without waiting for the vblank, which is the only way
	// past the refresh rate of the display, 1 locks the frame rate to it.
	void Present(const UINT SyncInterval = 1) const;

	// Whether the display chain can present without waiting for a vblank at all.
	// Remote desktop sessions and some compositors do not allow it.
	_NODISCARD FORCEINLINE bool IsTearingSupported() const noexcept
	{
		return bTearingSupported;
	}

	// Getters
	_NODISCARD FORCEINLINE IDXGISwapChain3* Get() const noexcept
	{
		return SwapChain.Get();
	}
	_NODISCARD FORCEINLINE UINT GetBackBufferCount() const noexcept
	{
		return BackBufferCount;
	}

private:

	ComPtr<IDXGISwapChain3>				SwapChain;					// the swap chain itself
	std::vector<ComPtr<DXResource>>		BackBuffers;				// one surface per back buffer
	ComPtr<DXDescriptorHeap>			RtvHeap;					// one render target view per back buffer
	UINT								RtvIncrementSize	= 0;	// size of a single render target descriptor on this device
	UINT								BackBufferCount		= 0;	// how many back buffers the chain was created with
	bool								bTearingSupported	= false;// whether an unsynchronized present is allowed at all
};

#endif // D3D12_SWAPCHAIN_H_INCLUDED
