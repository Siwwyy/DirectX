//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "ShadowMap.h"

// Namespaces
using namespace Helpers;

void ShadowMap::Initialize(DXDevice * Device, const UINT Resolution)
{
	DXASSERT(Device, "Device can not be null for Initialization of the shadow map");
	DXASSERT(Resolution > 0, "Shadow map needs a non empty resolution");

	this->Resolution	= Resolution;
	ViewPort			= CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(Resolution), static_cast<float>(Resolution));
	ScissorRect			= CD3DX12_RECT(0, 0, static_cast<LONG>(Resolution), static_cast<LONG>(Resolution));

	// Depth stencil view, this one stays on the CPU side
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc	= {};
	DsvHeapDesc.NumDescriptors				= 1;
	DsvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DsvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&DsvHeap)));
	NAME_D3D12_OBJECT(DsvHeap, L"Shadow Map DSV Heap");

	// clear value for depth, clearing is cheaper when the resource knows it up front
	D3D12_CLEAR_VALUE DepthOptimizedClearValue		= {};
	DepthOptimizedClearValue.Format					= GetFormat();
	DepthOptimizedClearValue.DepthStencil.Depth		= DEPTH_CLEAR_VALUE;
	DepthOptimizedClearValue.DepthStencil.Stencil	= 0;

	// The target has to allow both writing depth into it and sampling it afterwards
	const auto TargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		GetFormat(),
		Resolution,
		Resolution,
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	// Starts out sampleable, ShadowPass flips it to depth write
	CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT,	// a default heap, the GPU is the only one touching it
		D3D12_HEAP_FLAG_NONE,		// no flags
		&TargetDesc,				// the description of the depth target
		CurrentState,
		&DepthOptimizedClearValue,	// value Clear() writes
		IID_PPV_ARGS(&DepthTarget)));
	NAME_D3D12_OBJECT(DepthTarget, L"Shadow Map Depth Target");

	// Create Depth Stencil View
	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc	= {};
	DepthStencilViewDesc.Format							= GetFormat();
	DepthStencilViewDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
	DepthStencilViewDesc.Flags							= D3D12_DSV_FLAG_NONE;
	Device->CreateDepthStencilView(DepthTarget.Get(), &DepthStencilViewDesc, DsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void ShadowMap::CreateShaderResourceView(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle) const
{
	DXASSERT(Device, "Device can not be null for creating the shadow map view");
	DXASSERT(DepthTarget, "Shadow map must be initialized before creating its view");

	// A D32 target is sampled through its R32 view, the depth sits in the red channel
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format					= GetSrvFormat();
	SrvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels		= 1;
	Device->CreateShaderResourceView(DepthTarget.Get(), &SrvDesc, SrvHandle);
}

void ShadowMap::TransitionToDepthWrite(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the shadow map");

	constexpr auto StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	if (CurrentState == StateAfter)
	{
		return;
	}

	const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthTarget.Get(), CurrentState, StateAfter);
	CommandList->ResourceBarrier(1, &Barrier);
	CurrentState = StateAfter;
}

void ShadowMap::TransitionToShaderResource(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the shadow map");

	constexpr auto StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	if (CurrentState == StateAfter)
	{
		return;
	}

	const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthTarget.Get(), CurrentState, StateAfter);
	CommandList->ResourceBarrier(1, &Barrier);
	CurrentState = StateAfter;
}

void ShadowMap::BindAsDepthTarget(DXGraphicsCommandList * CommandList) const
{
	DXASSERT(CommandList, "Command list can not be null for binding the shadow map");
	DXASSERT(CurrentState == D3D12_RESOURCE_STATE_DEPTH_WRITE, "Shadow map must be transitioned to depth write before binding it");

	// Depth only, there is no colour to write during the shadow pass
	const auto DsvHandle = DsvHeap->GetCPUDescriptorHandleForHeapStart();
	CommandList->OMSetRenderTargets(0, nullptr, FALSE, &DsvHandle);
}

void ShadowMap::Clear(DXGraphicsCommandList * CommandList) const
{
	DXASSERT(CommandList, "Command list can not be null for clearing the shadow map");
	DXASSERT(CurrentState == D3D12_RESOURCE_STATE_DEPTH_WRITE, "Shadow map must be transitioned to depth write before clearing it");

	CommandList->ClearDepthStencilView(DsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, DEPTH_CLEAR_VALUE, 0, 0, nullptr);
}
