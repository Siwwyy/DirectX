#include "CommandListInterface.h"

#include <cassert>

using Helpers::ThrowIfFailed;

CommandListInterface::CommandListInterface(
	DXDevice* const device,
	const D3D12_COMMAND_LIST_TYPE commandListType,
	_In_opt_ DXCommandAllocator * const	commandAllocator)
{
	DXASSERT(device, "Device pointer must be provided");
	auto cmdAllocPtr = commandAllocator;
	if(!cmdAllocPtr)
	{
		// If Command Allocator has not been provided, lets create a one set directly to committed command list
		// Something like CommittedResource and implicit Heap
		ThrowIfFailed(device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&cmdAllocPtr)));
	}

	// Set pointer to the command allocator inside class
	//this->commandAllocator->QueryInterface(IID_PPV_ARGS(&cmdAllocPtr));
	this->commandAllocator = ComPtr<DXCommandAllocator>(cmdAllocPtr);
	this->commandAllocator->SetName(L"COMMAND ALLOCATOR");
}

void CommandListInterface::ResetCommandAllocator() const
{
	ThrowIfFailed(commandAllocator->Reset());
}
