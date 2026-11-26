//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_SWAPCHAIN_H_INCLUDED
#define DXI_SWAPCHAIN_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIDevice.h"

// Swap chain along with its back buffers and their render target views.
// Owns the barriers that flip a back buffer between presenting and being drawn into.
class DXISwapChain
{
	DISABLE_COPY_MOVE(DXISwapChain)

public:

	DXISwapChain()	= default;
	~DXISwapChain()	= default;

	/**
	 * Creates the swap chain for the given window and a render target view per back buffer
	 */
	void Init(const DXIDevice& Device, const HWND WindowHandle, const UINT Width, const UINT Height, const UINT BackBufferCount);

	// Format every back buffer is created with
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	/**
	 * Changes the size of every back buffer and makes their views again.
	 *
	 * The GPU must be idle: nothing in flight may still be drawing into or presenting the old
	 * buffers, see DXIFrameContext::WaitForGpuIdle. The back buffers come out in the present state
	 * like new ones, and which of them is current may change, so ask for the index afterwards.
	 */
	void Resize(const DXIDevice& Device, const UINT NewWidth, const UINT NewHeight);

	/**
	 * Index of the back buffer the next frame goes into
	 */
	_NODISCARD UINT GetCurrentBackBufferIdx() const;

	/**
	 * Render target view of one of the back buffers
	 */
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(const UINT BackBufferIdx) const;

	/**
	 * Barriers | A back buffer is presentable until a pass draws into it
	 */
	void TransitionToRenderTarget(DXGraphicsCommandList * CommandList, const UINT BackBufferIdx) const;
	void TransitionToPresent(DXGraphicsCommandList * CommandList, const UINT BackBufferIdx) const;

	/**
	 * Hands the finished frame over to the compositor.
	 * A SyncInterval of 0 presents without waiting for the vblank, which is the only way
	 * past the refresh rate of the display, 1 locks the frame rate to it.
	 */
	void Present(const UINT SyncInterval = 1) const;

	// Whether the display chain can present without waiting for a vblank at all.
	// Remote desktop sessions and some compositors do not allow it.
	_NODISCARD FORCEINLINE bool IsTearingSupported() const noexcept
	{
		return bTearingSupported;
	}

	// Getters
	_NODISCARD FORCEINLINE DXResource* GetBackBuffer(const UINT BackBufferIdx) const
	{
		return BackBuffers[BackBufferIdx].Get();
	}
	_NODISCARD FORCEINLINE IDXGISwapChain3* Get() const noexcept
	{
		return SwapChain.Get();
	}
	_NODISCARD FORCEINLINE UINT GetBackBufferCount() const noexcept
	{
		return BackBufferCount;
	}
	_NODISCARD FORCEINLINE UINT GetWidth() const noexcept
	{
		return Width;
	}
	_NODISCARD FORCEINLINE UINT GetHeight() const noexcept
	{
		return Height;
	}

private:

	/**
	 * Takes the back buffers out of the chain, names them and gives each its view
	 */
	void CreateBackBufferViews(const DXIDevice& Device);

	ComPtr<IDXGISwapChain3>				SwapChain;					// the swap chain itself
	std::vector<ComPtr<DXResource>>		BackBuffers;				// one surface per back buffer
	ComPtr<DXDescriptorHeap>			RtvHeap;					// one render target view per back buffer
	UINT								RtvIncrementSize	= 0;	// size of a single render target descriptor on this device
	UINT								BackBufferCount		= 0;	// how many back buffers the chain was created with
	UINT								Width				= 0;	// current size of the back buffers, in pixels
	UINT								Height				= 0;
	UINT								Flags				= 0;	// DXGI_SWAP_CHAIN_FLAG_* it was created with, ResizeBuffers has to repeat them
	bool								bTearingSupported	= false;// whether an unsynchronized present is allowed at all
};

#endif // DXI_SWAPCHAIN_H_INCLUDED
