//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_GBUFFER_H_INCLUDED
#define D3D12_GBUFFER_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"

// Render targets BasePass writes the surface into and LightPass reads back.
// The order here is the SV_Target order of BasePass and the register order of LightPass,
// i.e., Albedo is SV_Target0 and t0, Normal is SV_Target1 and t1 and so on.
enum class GBufferTarget : UINT
{
	Albedo = 0,				// Surface colour straight out of the material diffuse slot
	Normal = 1,				// World space normal
	WorldPosition = 2,		// World space position, w is 1 where geometry was written and 0 for the background
	MaterialParams = 3,		// r metalness, g roughness, b ambient occlusion | what drives the reflections

	Count
};

// How many render targets the G-buffer is made of
constexpr UINT GBufferTargetCount = static_cast<UINT>(GBufferTarget::Count);

// Target name, for debug purposes
_NODISCARD const wchar_t* ToString(const GBufferTarget Target);


// Off screen surface written by BasePass and consumed by LightPass.
// Owns the targets plus both a render target and a shader visible descriptor heap for them,
// and keeps track of which state the targets are in so the barriers stay balanced.
class GBuffer
{
	DISABLE_COPY_MOVE(GBuffer)

public:

	GBuffer()	= default;
	~GBuffer()	= default;

	// Creates the targets at the given size along with their descriptors
	void Initialize(DXDevice * Device, const UINT Width, const UINT Height);

	// Format of a target, the passes need these to build their PSOs
	_NODISCARD static DXGI_FORMAT GetFormat(const GBufferTarget Target);

	// Writes one shader resource view per target starting at the given slot, in the
	// GBufferTarget order. The caller owns the heap, because LightPass has to see these
	// views and the shadow map through one single shader visible heap.
	void CreateShaderResourceViews(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE StartHandle, const UINT IncrementSize) const;

	// Barriers | BasePass writes the targets, LightPass samples them
	void TransitionToRenderTarget(DXGraphicsCommandList * CommandList);
	void TransitionToShaderResource(DXGraphicsCommandList * CommandList);

	// Binds every target at once, BasePass draws into all of them
	void BindAsRenderTarget(DXGraphicsCommandList * CommandList, const D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle) const;

	// Clears every target, the background colour ends up in Albedo
	void Clear(DXGraphicsCommandList * CommandList) const;

private:

	// CPU handle of a single render target view
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(const GBufferTarget Target) const;

	ComPtr<DXResource>			Targets[GBufferTargetCount];	// the off screen surfaces themselves
	ComPtr<DXDescriptorHeap>	RtvHeap;						// one render target view per target
	UINT						RtvIncrementSize	= 0;		// size of a single render target descriptor on this device

	// State every target currently sits in, both transitions are no-ops when already there
	D3D12_RESOURCE_STATES		CurrentState		= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};

#endif // D3D12_GBUFFER_H_INCLUDED