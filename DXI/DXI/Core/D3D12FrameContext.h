//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_FRAMECONTEXT_H_INCLUDED
#define D3D12_FRAMECONTEXT_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "D3D12Device.h"

// Per frame command recording and CPU to GPU synchronization.
// Holds one command allocator and one fence per back buffer plus the single command
// list every pass records into.
class D3D12FrameContext
{
	DISABLE_COPY_MOVE(D3D12FrameContext)

public:

	D3D12FrameContext()		= default;
	~D3D12FrameContext();

	// Creates an allocator and a fence per frame, the command list and the fence event.
	// Leaves the command list recording, so startup uploads can go in right away.
	void Initialize(const D3D12Device& Device, const UINT FrameCount);

	// Reclaims the frame's allocator and reopens the command list for recording.
	// The list has to be closed when this is called, Reset() rejects a recording list.
	void BeginFrame(const UINT FrameIdx);

	// Closes the command list and submits it, leaving it closed
	void Execute(const D3D12Device& Device) const;

	// Submits whatever has been recorded so far and waits it out, leaving the list closed
	void Flush(const D3D12Device& Device, const UINT FrameIdx);

	// Blocks until the GPU is done with the given frame
	void WaitForFrame(const D3D12Device& Device, const UINT FrameIdx);

	// Getters
	_NODISCARD FORCEINLINE DXGraphicsCommandList* GetCommandList() const noexcept
	{
		return CommandList.Get();
	}

private:

	ComPtr<DXGraphicsCommandList>			CommandList;		// the one list every pass records into
	std::vector<ComPtr<DXCommandAllocator>>	CommandAllocators;	// one per frame, reset when that frame comes around again
	std::vector<ComPtr<DXFence>>			Fences;				// one per frame
	std::vector<UINT64>						FenceValues;		// value each fence is waiting for
	HANDLE									FenceEvent = nullptr;	// event the CPU blocks on
};

#endif // D3D12_FRAMECONTEXT_H_INCLUDED
