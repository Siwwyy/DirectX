//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "D3D12FrameContext.h"

// Namespaces
using namespace Helpers;

// Direct queue, everything this engine submits is graphics work
static constexpr D3D12_COMMAND_LIST_TYPE COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;

D3D12FrameContext::~D3D12FrameContext()
{
	if (FenceEvent)
	{
		CloseHandle(FenceEvent);
		FenceEvent = nullptr;
	}
}

void D3D12FrameContext::Initialize(const D3D12Device& Device, const UINT FrameCount)
{
	DXASSERT(Device.IsValid(), "Device must be initialized before the frame context");
	DXASSERT(FrameCount > 0, "Frame context needs at least one frame");

	CommandAllocators.resize(FrameCount);
	Fences.resize(FrameCount);
	FenceValues.resize(FrameCount);

	for (UINT FrameIdx = 0; FrameIdx < FrameCount; ++FrameIdx)
	{
		ThrowIfFailed(Device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fences[FrameIdx])));
		ThrowIfFailed(Device.Get()->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&CommandAllocators[FrameIdx])));
		FenceValues[FrameIdx] = 0;
	}

	// Create an event handle to use for frame synchronization.
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// CreateCommandList hands back a list that is already recording against the first
	// allocator, so initialization work can be recorded straight away
	CommandList = CreateGraphicsCommandList(
		Device.Get(),
		CommandListDesc{ 0, COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAG_NONE },
		CommandAllocators[0].Get(),
		nullptr);
	NAME_D3D12_OBJECT(CommandList, L"Main Command List");
}

void D3D12FrameContext::BeginFrame(const D3D12Device& Device, const UINT FrameIdx)
{
	DXASSERT(CommandList, "Frame context must be initialized before recording a frame");
	DXASSERT(FrameIdx < CommandAllocators.size(), "Frame index is out of range");

	// An allocator can only be reset once the GPU is done with everything recorded from
	// it, so this is the point the CPU has to catch up. With BACK_BUFFER_COUNT frames in
	// flight it is usually already finished and this returns straight away.
	WaitForFrame(Device, FrameIdx);

	ThrowIfFailed(CommandAllocators[FrameIdx]->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocators[FrameIdx].Get(), nullptr));
}

void D3D12FrameContext::Execute(const D3D12Device& Device, const UINT FrameIdx)
{
	DXASSERT(CommandList, "Frame context must be initialized before submitting a frame");
	DXASSERT(FrameIdx < Fences.size(), "Frame index is out of range");

	ThrowIfFailed(CommandList->Close()); //close command list for execution

	DXCommandList* CommandLists[] = { CommandList.Get() };
	Device.GetCommandQueue()->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	// Mark where this frame ends on the queue. Nothing blocks here, the wait happens when
	// this frame index comes back around in BeginFrame().
	FenceValues[FrameIdx] = FenceValues[FrameIdx] + 1;
	ThrowIfFailed(Device.GetCommandQueue()->Signal(Fences[FrameIdx].Get(), FenceValues[FrameIdx]));
}

void D3D12FrameContext::Flush(const D3D12Device& Device, const UINT FrameIdx)
{
	DXASSERT(FrameIdx < CommandAllocators.size(), "Frame index is out of range");

	// Leaves the list closed on purpose. Reset() only accepts a closed list, so the
	// next BeginFrame() is what reopens it and reclaims that frame's allocator.
	Execute(Device, FrameIdx);
	WaitForFrame(Device, FrameIdx);
}

void D3D12FrameContext::WaitForGpuIdle(const D3D12Device& Device)
{
	if (!FenceEvent)
	{
		return;
	}

	// The queue runs in order, so waiting for the last thing signalled on every frame
	// covers everything still in flight
	for (UINT FrameIdx = 0; FrameIdx < static_cast<UINT>(Fences.size()); ++FrameIdx)
	{
		WaitForFrame(Device, FrameIdx);
	}
}

void D3D12FrameContext::WaitForFrame(const D3D12Device& Device, const UINT FrameIdx)
{
	DXASSERT(FrameIdx < Fences.size(), "Frame index is out of range");

	// Execute() is what signals, this only blocks until that point is reached.
	// A frame that has never been submitted has a value of 0 and never waits.
	if (Fences[FrameIdx]->GetCompletedValue() < FenceValues[FrameIdx])
	{
		ThrowIfFailed(Fences[FrameIdx]->SetEventOnCompletion(FenceValues[FrameIdx], FenceEvent));
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}
