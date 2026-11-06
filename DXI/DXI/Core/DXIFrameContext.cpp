//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "DXIFrameContext.h"

// Namespaces
using namespace Helpers;

// Direct queue, everything this engine submits is graphics work
static constexpr D3D12_COMMAND_LIST_TYPE COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;

DXIFrameContext::~DXIFrameContext()
{
	if (FenceEvent)
	{
		CloseHandle(FenceEvent);
		FenceEvent = nullptr;
	}
}

void DXIFrameContext::Init(const DXIDevice& Device, const UINT FrameCount)
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

void DXIFrameContext::BeginFrame(const DXIDevice& Device, const UINT FrameIdx)
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

void DXIFrameContext::Execute(const DXIDevice& Device, const UINT FrameIdx)
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

void DXIFrameContext::Flush(const DXIDevice& Device, const UINT FrameIdx)
{
	DXASSERT(FrameIdx < CommandAllocators.size(), "Frame index is out of range");

	// Leaves the list closed on purpose. Reset() only accepts a closed list, so the
	// next BeginFrame() is what reopens it and reclaims that frame's allocator.
	Execute(Device, FrameIdx);
	WaitForFrame(Device, FrameIdx);
}

void DXIFrameContext::WaitForGpuIdle(const DXIDevice& Device)
{
	if (!FenceEvent || Fences.empty())
	{
		return;
	}

	// Catch up on every frame that has been submitted
	for (UINT FrameIdx = 0; FrameIdx < static_cast<UINT>(Fences.size()); ++FrameIdx)
	{
		WaitForFrame(Device, FrameIdx);
	}

	// Those waits are not enough on their own. Execute() signals a frame's fence and
	// EndFrame() calls Present afterwards, so the last Present sits on the queue *behind*
	// the newest signal and nothing above waits for it. Releasing the swap chain then
	// pulls a back buffer out from under a present that is still running, which the debug
	// layer reports as OBJECT_DELETED_WHILE_STILL_IN_USE.
	//
	// A fresh signal goes to the back of the queue, after that Present, so waiting on it
	// covers everything. One fence is enough because a queue completes in order.
	FenceValues[0] = FenceValues[0] + 1;
	ThrowIfFailed(Device.GetCommandQueue()->Signal(Fences[0].Get(), FenceValues[0]));
	WaitForFrame(Device, 0);
}

void DXIFrameContext::WaitForFrame(const DXIDevice& Device, const UINT FrameIdx)
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
