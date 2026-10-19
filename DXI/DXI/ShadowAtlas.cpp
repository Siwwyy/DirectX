//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "ShadowAtlas.h"

// Namespaces
using namespace Helpers;

// Nothing occludes until something is drawn, so an untouched face has to read as "the
// light reaches all the way out"
static constexpr float DISTANCE_CLEAR_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };

void ShadowAtlas::Initialize(DXDevice * Device, const UINT Resolution, const UINT CubeResolution)
{
	DXASSERT(Device, "Device can not be null for Initialization of the shadow atlas");
	DXASSERT(Resolution > 0 && CubeResolution > 0, "Shadow atlas needs a non empty resolution");

	this->Resolution		= Resolution;
	this->CubeResolution	= CubeResolution;

	ViewPort		= CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(Resolution), static_cast<float>(Resolution));
	ScissorRect		= CD3DX12_RECT(0, 0, static_cast<LONG>(Resolution), static_cast<LONG>(Resolution));
	CubeViewPort	= CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(CubeResolution), static_cast<float>(CubeResolution));
	CubeScissorRect	= CD3DX12_RECT(0, 0, static_cast<LONG>(CubeResolution), static_cast<LONG>(CubeResolution));

	DsvIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	RtvIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// One depth view per light, plus one more for the depth the cube faces share
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc	= {};
		HeapDesc.NumDescriptors				= MAX_LIGHTS;
		HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&DepthDsvHeap)));
		NAME_D3D12_OBJECT(DepthDsvHeap, L"Shadow Atlas Depth DSV Heap");

		HeapDesc.NumDescriptors				= 1;
		ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&CubeDsvHeap)));
		NAME_D3D12_OBJECT(CubeDsvHeap, L"Shadow Atlas Cube Depth DSV Heap");
	}

	// One render target per cube face of every light
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc	= {};
		HeapDesc.NumDescriptors				= MAX_LIGHTS * CUBE_FACE_COUNT;
		HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&CubeRtvHeap)));
		NAME_D3D12_OBJECT(CubeRtvHeap, L"Shadow Atlas Cube RTV Heap");
	}

	D3D12_CLEAR_VALUE DepthClear		= {};
	DepthClear.Format					= GetDepthFormat();
	DepthClear.DepthStencil.Depth		= DEPTH_CLEAR_VALUE;

	// Depth array | one slice per directional or area light
	{
		const auto Desc = CD3DX12_RESOURCE_DESC::Tex2D(
			GetDepthFormat(),
			Resolution,
			Resolution,
			static_cast<UINT16>(MAX_LIGHTS),	// array size, a slice per light
			1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		ThrowIfFailed(Device->CreateCommittedResource(
			&DX_HEAP_PROPERTY_DEFAULT,
			D3D12_HEAP_FLAG_NONE,
			&Desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&DepthClear,
			IID_PPV_ARGS(&DepthArray)));
		NAME_D3D12_OBJECT(DepthArray, L"Shadow Atlas Depth Array");

		for (UINT Slot = 0; Slot < MAX_LIGHTS; ++Slot)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc	= {};
			DsvDesc.Format							= GetDepthFormat();
			DsvDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			DsvDesc.Texture2DArray.FirstArraySlice	= Slot;
			DsvDesc.Texture2DArray.ArraySize		= 1;
			Device->CreateDepthStencilView(DepthArray.Get(), &DsvDesc, GetDepthDsvHandle(Slot));
		}
	}

	// Distance cube array | six faces per point light, holding how far the light reaches
	{
		const auto Desc = CD3DX12_RESOURCE_DESC::Tex2D(
			GetDistanceFormat(),
			CubeResolution,
			CubeResolution,
			static_cast<UINT16>(MAX_LIGHTS * CUBE_FACE_COUNT),	// six slices make one cube
			1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		D3D12_CLEAR_VALUE DistanceClear	= {};
		DistanceClear.Format			= GetDistanceFormat();
		memcpy(DistanceClear.Color, DISTANCE_CLEAR_COLOR, sizeof(DistanceClear.Color));

		ThrowIfFailed(Device->CreateCommittedResource(
			&DX_HEAP_PROPERTY_DEFAULT,
			D3D12_HEAP_FLAG_NONE,
			&Desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&DistanceClear,
			IID_PPV_ARGS(&DistanceCubeArray)));
		NAME_D3D12_OBJECT(DistanceCubeArray, L"Shadow Atlas Distance Cube Array");

		for (UINT Slot = 0; Slot < MAX_LIGHTS; ++Slot)
		{
			for (UINT FaceIdx = 0; FaceIdx < CUBE_FACE_COUNT; ++FaceIdx)
			{
				D3D12_RENDER_TARGET_VIEW_DESC RtvDesc	= {};
				RtvDesc.Format							= GetDistanceFormat();
				RtvDesc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				RtvDesc.Texture2DArray.FirstArraySlice	= (Slot * CUBE_FACE_COUNT) + FaceIdx;
				RtvDesc.Texture2DArray.ArraySize		= 1;
				Device->CreateRenderTargetView(DistanceCubeArray.Get(), &RtvDesc, GetCubeRtvHandle(Slot, FaceIdx));
			}
		}
	}

	// One depth buffer shared by every cube face. Faces are rendered one at a time and
	// it is cleared before each, so they never need separate ones.
	{
		const auto Desc = CD3DX12_RESOURCE_DESC::Tex2D(
			GetDepthFormat(),
			CubeResolution,
			CubeResolution,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		ThrowIfFailed(Device->CreateCommittedResource(
			&DX_HEAP_PROPERTY_DEFAULT,
			D3D12_HEAP_FLAG_NONE,
			&Desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&DepthClear,
			IID_PPV_ARGS(&CubeDepth)));
		NAME_D3D12_OBJECT(CubeDepth, L"Shadow Atlas Cube Depth");

		D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc	= {};
		DsvDesc.Format							= GetDepthFormat();
		DsvDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
		Device->CreateDepthStencilView(CubeDepth.Get(), &DsvDesc, CubeDsvHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void ShadowAtlas::CreateShaderResourceViews(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE StartHandle, const UINT IncrementSize) const
{
	DXASSERT(Device, "Device can not be null for creating the shadow atlas views");
	DXASSERT(DepthArray && DistanceCubeArray, "Shadow atlas must be initialized before creating its views");

	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(StartHandle);

	// Depth slices, read through their R32 view
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc		= {};
		SrvDesc.Shader4ComponentMapping				= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format								= GetDepthSrvFormat();
		SrvDesc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		SrvDesc.Texture2DArray.MipLevels			= 1;
		SrvDesc.Texture2DArray.ArraySize			= MAX_LIGHTS;
		Device->CreateShaderResourceView(DepthArray.Get(), &SrvDesc, Handle);
	}

	Handle.Offset(1, IncrementSize);

	// Cube array. Declaring it as a cube is what lets the shader index it with a
	// direction, the six slices behind each cube are then picked for free.
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc		= {};
		SrvDesc.Shader4ComponentMapping				= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format								= GetDistanceFormat();
		SrvDesc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		SrvDesc.TextureCubeArray.MipLevels			= 1;
		SrvDesc.TextureCubeArray.NumCubes			= MAX_LIGHTS;
		Device->CreateShaderResourceView(DistanceCubeArray.Get(), &SrvDesc, Handle);
	}
}

void ShadowAtlas::TransitionToWrite(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the shadow atlas");

	if (bWritable)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER Barriers[2] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(DepthArray.Get(),			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
		CD3DX12_RESOURCE_BARRIER::Transition(DistanceCubeArray.Get(),	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
	};

	CommandList->ResourceBarrier(_countof(Barriers), Barriers);
	bWritable = true;
}

void ShadowAtlas::TransitionToShaderResource(DXGraphicsCommandList * CommandList)
{
	DXASSERT(CommandList, "Command list can not be null for transitioning the shadow atlas");

	if (!bWritable)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER Barriers[2] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(DepthArray.Get(),			D3D12_RESOURCE_STATE_DEPTH_WRITE,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(DistanceCubeArray.Get(),	D3D12_RESOURCE_STATE_RENDER_TARGET,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	};

	CommandList->ResourceBarrier(_countof(Barriers), Barriers);
	bWritable = false;
}

void ShadowAtlas::BindDepthSlice(DXGraphicsCommandList * CommandList, const UINT Slot) const
{
	DXASSERT(CommandList, "Command list can not be null for binding a shadow slice");
	DXASSERT(Slot < MAX_LIGHTS, "Shadow slot is outside the atlas");
	DXASSERT(bWritable, "Shadow atlas must be transitioned to write before binding a slice");

	const auto DsvHandle = GetDepthDsvHandle(Slot);

	// Depth only, a directional or area light has no colour to write
	CommandList->OMSetRenderTargets(0, nullptr, FALSE, &DsvHandle);
	CommandList->ClearDepthStencilView(DsvHandle, D3D12_CLEAR_FLAG_DEPTH, DEPTH_CLEAR_VALUE, 0, 0, nullptr);

	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);
}

void ShadowAtlas::BindCubeFace(DXGraphicsCommandList * CommandList, const UINT Slot, const UINT FaceIdx) const
{
	DXASSERT(CommandList, "Command list can not be null for binding a shadow face");
	DXASSERT(Slot < MAX_LIGHTS, "Shadow slot is outside the atlas");
	DXASSERT(FaceIdx < CUBE_FACE_COUNT, "A cube has six faces, the index has to be one of them");
	DXASSERT(bWritable, "Shadow atlas must be transitioned to write before binding a face");

	const auto RtvHandle = GetCubeRtvHandle(Slot, FaceIdx);
	const auto DsvHandle = CubeDsvHeap->GetCPUDescriptorHandleForHeapStart();

	CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, &DsvHandle);
	CommandList->ClearRenderTargetView(RtvHandle, DISTANCE_CLEAR_COLOR, 0, nullptr);
	CommandList->ClearDepthStencilView(DsvHandle, D3D12_CLEAR_FLAG_DEPTH, DEPTH_CLEAR_VALUE, 0, 0, nullptr);

	CommandList->RSSetViewports(1, &CubeViewPort);
	CommandList->RSSetScissorRects(1, &CubeScissorRect);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowAtlas::GetDepthDsvHandle(const UINT Slot) const
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(DepthDsvHeap->GetCPUDescriptorHandleForHeapStart());
	Handle.Offset(Slot, DsvIncrementSize);
	return Handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowAtlas::GetCubeRtvHandle(const UINT Slot, const UINT FaceIdx) const
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(CubeRtvHeap->GetCPUDescriptorHandleForHeapStart());
	Handle.Offset((Slot * CUBE_FACE_COUNT) + FaceIdx, RtvIncrementSize);
	return Handle;
}
