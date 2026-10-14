//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "D3D12SwapChain.h"

// Namespaces
using namespace Helpers;

void D3D12SwapChain::Initialize(const D3D12Device& Device, const HWND WindowHandle, const UINT Width, const UINT Height, const UINT BackBufferCount)
{
	DXASSERT(Device.IsValid(), "Device must be initialized before the swap chain");
	DXASSERT(WindowHandle, "Swap chain needs a window to present into");
	DXASSERT(BackBufferCount > 0, "Swap chain needs at least one back buffer");

	this->BackBufferCount = BackBufferCount;

	// Presenting without waiting for a vblank needs the chain to be created for it.
	// It is unavailable over remote desktop, so the result decides whether an
	// unsynchronized Present() can do anything at all.
	{
		ComPtr<IDXGIFactory5> Factory5;
		BOOL bAllowTearing = FALSE;
		if (SUCCEEDED(Device.GetFactory()->QueryInterface(IID_PPV_ARGS(&Factory5)))
			&& SUCCEEDED(Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing))))
		{
			bTearingSupported = bAllowTearing == TRUE;
		}
	}

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc		= {};
	SwapChainDesc.BufferCount				= BackBufferCount;
	SwapChainDesc.Width						= Width;
	SwapChainDesc.Height					= Height;
	SwapChainDesc.Format					= GetFormat();
	SwapChainDesc.BufferUsage				= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.SwapEffect				= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.SampleDesc.Count			= 1;
	SwapChainDesc.Flags						= bTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> SwapChain1;
	ThrowIfFailed(Device.GetFactory()->CreateSwapChainForHwnd(
		Device.GetCommandQueue(),	// Swap chain needs the queue so that it can force a flush on it.
		WindowHandle,
		&SwapChainDesc,
		nullptr,
		nullptr,
		&SwapChain1
	));

	// Convert SwapChain1 interface to SwapChain3
	ThrowIfFailed(SwapChain1.As(&SwapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(Device.GetFactory()->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER));

	// Describe and create a render target view (RTV) descriptor heap.
	constexpr auto DescriptorHeapType		= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc	= {};
	RtvHeapDesc.NumDescriptors				= BackBufferCount;
	RtvHeapDesc.Type						= DescriptorHeapType;
	RtvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(Device.Get()->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));
	NAME_D3D12_OBJECT(RtvHeap, L"Back Buffer RTV Heap");

	RtvIncrementSize = Device.Get()->GetDescriptorHandleIncrementSize(DescriptorHeapType);

	// One render target view per back buffer
	BackBuffers.resize(BackBufferCount);
	CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT BackBufferIdx = 0; BackBufferIdx < BackBufferCount; ++BackBufferIdx)
	{
		ThrowIfFailed(SwapChain->GetBuffer(BackBufferIdx, IID_PPV_ARGS(&BackBuffers[BackBufferIdx])));
		Device.Get()->CreateRenderTargetView(BackBuffers[BackBufferIdx].Get(), nullptr, RtvHandle);
		RtvHandle.Offset(1, RtvIncrementSize);
	}
}

UINT D3D12SwapChain::GetCurrentBackBufferIdx() const
{
	DXASSERT(SwapChain, "Swap chain must be initialized before asking for its back buffer index");

	return SwapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetRtvHandle(const UINT BackBufferIdx) const
{
	DXASSERT(RtvHeap, "Swap chain must be initialized before taking a handle out of it");
	DXASSERT(BackBufferIdx < BackBufferCount, "Back buffer index is out of range");

	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
	Handle.Offset(BackBufferIdx, RtvIncrementSize);
	return Handle;
}

void D3D12SwapChain::TransitionToRenderTarget(DXGraphicsCommandList * CommandList, const UINT BackBufferIdx) const
{
	DXASSERT(CommandList, "Command list can not be null for transitioning a back buffer");
	DXASSERT(BackBufferIdx < BackBufferCount, "Back buffer index is out of range");

	const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIdx].Get(),
																D3D12_RESOURCE_STATE_PRESENT,
																D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &Barrier);
}

void D3D12SwapChain::TransitionToPresent(DXGraphicsCommandList * CommandList, const UINT BackBufferIdx) const
{
	DXASSERT(CommandList, "Command list can not be null for transitioning a back buffer");
	DXASSERT(BackBufferIdx < BackBufferCount, "Back buffer index is out of range");

	const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIdx].Get(),
																D3D12_RESOURCE_STATE_RENDER_TARGET,
																D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &Barrier);
}

void D3D12SwapChain::Present(const UINT SyncInterval) const
{
	DXASSERT(SwapChain, "Swap chain must be initialized before presenting");

	// The tearing flag is only legal with no synchronization, and only when the chain
	// was created for it
	const UINT PresentFlags = (SyncInterval == 0 && bTearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;

	ThrowIfFailed(SwapChain->Present(SyncInterval, PresentFlags));
}
