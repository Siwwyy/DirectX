#include "CommandQueue.h"

#include <cassert>

using Helpers::ThrowIfFailed;

CommandQueue::CommandQueue(DXDevice* const device, const D3D12_COMMAND_LIST_TYPE commandListType)
	: fenceValue(0)
{
	assert(!device && "Device pointer must be provided");

	// Initialize command queue
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Flags			= D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask		= 0; //single GPU env for now
	commandQueueDesc.Priority		= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; 
	commandQueueDesc.Type			= commandListType;

	ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));

	// Initialize Fence event
	ThrowIfFailed(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Create an event handle to use for frame synchronization.
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void CommandQueue::WaitForFence()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE!!

	// Signal and increment the fence value.
	fenceValue++;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

	// Wait until recorded commands are processed by GPU.
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}