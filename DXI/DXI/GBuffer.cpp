//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "GBuffer.h"

// Namespaces
using namespace Helpers;

// Background of the scene. Albedo is cleared to it so pixels without geometry
// come out of LightPass with this colour instead of being shaded.
static constexpr float GBUFFER_BACKGROUND_COLOR[] = { 0.0f, 0.5f, 1.0f, 1.0f };

// Normal and world position start empty, a world position with w == 0 marks the background
static constexpr float GBUFFER_EMPTY_COLOR[] = { 0.0f, 0.0f, 0.0f, 0.0f };

const wchar_t* ToString(const GBufferTarget Target)
{
	switch (Target)
	{
	case GBufferTarget::Albedo:			return L"Albedo";
	case GBufferTarget::Normal:			return L"Normal";
	case GBufferTarget::WorldPosition:	return L"WorldPosition";
	case GBufferTarget::MaterialParams:	return L"MaterialParams";
	default:							return L"Unknown";
	}
}

DXGI_FORMAT GBuffer::GetFormat(const GBufferTarget Target)
{
	switch (Target)
	{
	case GBufferTarget::Albedo:			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;			// colour, 8 bits per channel is plenty
	case GBufferTarget::Normal:			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;		// signed, so it needs a float format | maybe change to DXGI_FORMAT_R32G32B32A32_FLOAT for better precision
	case GBufferTarget::WorldPosition:	return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;		// full precision, positions leave the unit range
	case GBufferTarget::MaterialParams:	return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;			// three factors in the unit range
	default:							return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	}
}

void GBuffer::Initialize(DXDevice * Device, const UINT Width, const UINT Height)
{
	DXASSERT(Device, "Device can not be null for Initialization of the G-buffer");
	DXASSERT(Width > 0 && Height > 0, "G-buffer needs a non empty size");

	// Render target views, these stay on the CPU side
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc	= {};
	RtvHeapDesc.NumDescriptors				= GBufferTargetCount;
	RtvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RtvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));
	NAME_D3D12_OBJECT(RtvHeap, L"G-Buffer RTV Heap");

	RtvIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// The targets start out ready to be sampled, BasePass transitions them before writing
	CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		const auto Target	= static_cast<GBufferTarget>(TargetIdx);
		const auto Format	= GetFormat(Target);

		// Clearing is cheaper when the resource knows the value up front
		D3D12_CLEAR_VALUE ClearValue = {};
		ClearValue.Format = Format;
		memcpy(ClearValue.Color,
				Target == GBufferTarget::Albedo ? GBUFFER_BACKGROUND_COLOR : GBUFFER_EMPTY_COLOR,
				sizeof(ClearValue.Color));

		const auto TargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			Format,
			Width,
			Height,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		ThrowIfFailed(Device->CreateCommittedResource(
			&DX_HEAP_PROPERTY_DEFAULT,	// a default heap, the GPU is the only one touching these
			D3D12_HEAP_FLAG_NONE,		// no flags
			&TargetDesc,				// the description of the target
			CurrentState,				// starts out sampleable, BasePass flips it to render target
			&ClearValue,				// value Clear() writes
			IID_PPV_ARGS(&Targets[TargetIdx])));
		Targets[TargetIdx]->SetName((std::wstring(L"G-Buffer ") + ToString(Target)).c_str());

		// Render target view
		Device->CreateRenderTargetView(Targets[TargetIdx].Get(), nullptr, GetRtvHandle(Target));
	}
}

void GBuffer::CreateShaderResourceViews(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE StartHandle, const UINT IncrementSize) const
{
	DXASSERT(Device, "Device can not be null for creating the G-buffer views");
	DXASSERT(Targets[0], "G-buffer must be initialized before creating its views");

	CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHandle(StartHandle);

	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format					= GetFormat(static_cast<GBufferTarget>(TargetIdx));
		SrvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Texture2D.MipLevels		= 1;
		Device->CreateShaderResourceView(Targets[TargetIdx].Get(), &SrvDesc, SrvHandle);

		SrvHandle.Offset(1, IncrementSize);
	}
}

void GBuffer::TransitionToRenderTarget(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the G-buffer");

	constexpr auto StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	if (CurrentState == StateAfter)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER Barriers[GBufferTargetCount] = {};
	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		Barriers[TargetIdx] = CD3DX12_RESOURCE_BARRIER::Transition(Targets[TargetIdx].Get(), CurrentState, StateAfter);
	}

	CommandList->ResourceBarrier(GBufferTargetCount, Barriers);
	CurrentState = StateAfter;
}

void GBuffer::TransitionToShaderResource(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the G-buffer");

	constexpr auto StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	if (CurrentState == StateAfter)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER Barriers[GBufferTargetCount] = {};
	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		Barriers[TargetIdx] = CD3DX12_RESOURCE_BARRIER::Transition(Targets[TargetIdx].Get(), CurrentState, StateAfter);
	}

	CommandList->ResourceBarrier(GBufferTargetCount, Barriers);
	CurrentState = StateAfter;
}

void GBuffer::BindAsRenderTarget(DXGraphicsCommandList * CommandList, const D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle) const
{
	DXASSERT(CommandList, "Command list can not be null for binding the G-buffer");
	DXASSERT(CurrentState == D3D12_RESOURCE_STATE_RENDER_TARGET, "G-buffer must be transitioned to render target before binding it as one");

	D3D12_CPU_DESCRIPTOR_HANDLE RtvHandles[GBufferTargetCount] = {};
	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		RtvHandles[TargetIdx] = GetRtvHandle(static_cast<GBufferTarget>(TargetIdx));
	}

	CommandList->OMSetRenderTargets(GBufferTargetCount, RtvHandles, FALSE, &DsvHandle);
}

void GBuffer::Clear(DXGraphicsCommandList * CommandList) const
{
	DXASSERT(CommandList, "Command list can not be null for clearing the G-buffer");
	DXASSERT(CurrentState == D3D12_RESOURCE_STATE_RENDER_TARGET, "G-buffer must be transitioned to render target before clearing it");

	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		const auto Target = static_cast<GBufferTarget>(TargetIdx);
		CommandList->ClearRenderTargetView(GetRtvHandle(Target),
											Target == GBufferTarget::Albedo ? GBUFFER_BACKGROUND_COLOR : GBUFFER_EMPTY_COLOR,
											0,
											nullptr);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE GBuffer::GetRtvHandle(const GBufferTarget Target) const
{
	DXASSERT(RtvHeap, "G-buffer must be initialized before taking a handle out of it");
	DXASSERT(Target < GBufferTarget::Count, "Unknown G-buffer target");

	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
	Handle.Offset(static_cast<UINT>(Target), RtvIncrementSize);
	return Handle;
}
