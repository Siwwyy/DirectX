
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include "D3D12Helpers.h"
#include "d3dx12.h"



class CommandQueue
{
public:

	DISABLE_COPY(CommandQueue)

	CommandQueue() = default;
	~CommandQueue() = default;

	CommandQueue(DXDevice* const device, const D3D12_COMMAND_LIST_TYPE commandListType);

	// Utility functions
	void WaitForFence();
	void ExecuteCommandLists(DXCommandList* commandLists[]) const;

	[[nodiscard]] DXCommandQueue* GetCommandQueue() const noexcept { return commandQueue.Get(); }
	[[nodiscard]] DXFence* GetFence() const noexcept { return fence.Get(); }

private:

	ComPtr<DXCommandQueue>			commandQueue;
	ComPtr<DXFence>					fence;
	UINT64							fenceValue;
	HANDLE							fenceEvent;

};