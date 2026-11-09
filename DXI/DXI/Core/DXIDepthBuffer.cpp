//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "DXIDepthBuffer.h"

// Namespaces
using namespace Helpers;

void DXIDepthBuffer::Init(const DXIDevice& Device, const UINT Width, const UINT Height)
{
	DXASSERT(Device.IsValid(), "Device must be initialized before the depth buffer");
	DXASSERT(Width > 0 && Height > 0, "Depth buffer needs a non empty size");

	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc	= {};
	DsvHeapDesc.NumDescriptors				= 1;
	DsvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DsvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(Device.Get()->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&DsvHeap)));
	NAME_D3D12_OBJECT(DsvHeap, L"Depth Stencil DSV Heap");

	// clear value for depth stencil, clearing is cheaper when the resource knows it up front
	D3D12_CLEAR_VALUE DepthOptimizedClearValue		= {};
	DepthOptimizedClearValue.Format					= GetFormat();
	DepthOptimizedClearValue.DepthStencil.Depth		= DEPTH_CLEAR_VALUE;
	DepthOptimizedClearValue.DepthStencil.Stencil	= 0;

	// Create Depth Stencil Texture 2D.
	//
	// Typeless, so the same memory can carry a D32_FLOAT view to render through and an
	// R32_FLOAT view to sample. Note the absence of DENY_SHADER_RESOURCE: a depth target is
	// often created with it, and that flag would make the shader resource view below
	// illegal. Mip count of 0 means "as many as fit", which is one at this size.
	const auto DepthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		GetResourceFormat(),
		Width,
		Height,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	CurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	ThrowIfFailed(Device.Get()->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT,
		D3D12_HEAP_FLAG_NONE,
		&DepthStencilDesc,
		CurrentState,
		&DepthOptimizedClearValue,
		IID_PPV_ARGS(&DepthStencil)));
	NAME_D3D12_OBJECT(DepthStencil, L"Depth Stencil Buffer");

	// create a depth stencil view
	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc	= {};
	DepthStencilViewDesc.Format							= GetFormat();
	DepthStencilViewDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
	DepthStencilViewDesc.Flags							= D3D12_DSV_FLAG_NONE;
	Device.Get()->CreateDepthStencilView(DepthStencil.Get(), &DepthStencilViewDesc, DsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DXIDepthBuffer::Clear(DXGraphicsCommandList * CommandList) const
{
	DXASSERT(CommandList, "Command list can not be null for clearing the depth buffer");
	DXASSERT(DsvHeap, "Depth buffer must be initialized before clearing it");
	DXASSERT(CurrentState == D3D12_RESOURCE_STATE_DEPTH_WRITE, "Depth buffer has to be in the depth write state to be cleared, call TransitionToDepthWrite first");

	CommandList->ClearDepthStencilView(GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, DEPTH_CLEAR_VALUE, 0, 0, nullptr);
}

void DXIDepthBuffer::CreateShaderResourceView(const DXIDevice& Device, const D3D12_CPU_DESCRIPTOR_HANDLE Handle) const
{
	DXASSERT(Device.IsValid(), "Device must be initialized before creating the depth view");
	DXASSERT(DepthStencil, "Depth buffer must be initialized before creating its views");

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc	= {};
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format							= GetSrvFormat();		// the typed read of the typeless resource
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels				= 1;

	Device.Get()->CreateShaderResourceView(DepthStencil.Get(), &SrvDesc, Handle);
}

void DXIDepthBuffer::TransitionToDepthWrite(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the depth buffer");

	constexpr auto StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	if (CurrentState == StateAfter)
	{
		return;
	}

	const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencil.Get(), CurrentState, StateAfter);
	CommandList->ResourceBarrier(1, &Barrier);
	CurrentState = StateAfter;
}

void DXIDepthBuffer::TransitionToShaderResource(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the depth buffer");

	constexpr auto StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	if (CurrentState == StateAfter)
	{
		return;
	}

	const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencil.Get(), CurrentState, StateAfter);
	CommandList->ResourceBarrier(1, &Barrier);
	CurrentState = StateAfter;
}

D3D12_CPU_DESCRIPTOR_HANDLE DXIDepthBuffer::GetDsvHandle() const
{
	DXASSERT(DsvHeap, "Depth buffer must be initialized before taking its handle");

	return DsvHeap->GetCPUDescriptorHandleForHeapStart();
}
