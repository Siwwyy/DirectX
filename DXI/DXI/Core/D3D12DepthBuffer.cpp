//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "D3D12DepthBuffer.h"

// Namespaces
using namespace Helpers;

void D3D12DepthBuffer::Initialize(const D3D12Device& Device, const UINT Width, const UINT Height)
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

	// Create Depth Stencil Texture 2D
	const auto DepthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		GetFormat(),
		Width,
		Height,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(Device.Get()->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT,
		D3D12_HEAP_FLAG_NONE,
		&DepthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
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

void D3D12DepthBuffer::Clear(DXGraphicsCommandList * CommandList) const
{
	DXASSERT(CommandList, "Command list can not be null for clearing the depth buffer");
	DXASSERT(DsvHeap, "Depth buffer must be initialized before clearing it");

	CommandList->ClearDepthStencilView(GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, DEPTH_CLEAR_VALUE, 0, 0, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DepthBuffer::GetDsvHandle() const
{
	DXASSERT(DsvHeap, "Depth buffer must be initialized before taking its handle");

	return DsvHeap->GetCPUDescriptorHandleForHeapStart();
}
