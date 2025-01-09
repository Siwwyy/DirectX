
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Utils.h"

using Helpers::ThrowIfFailed;

DXGraphicsCommandList* Utils::CreateGraphicsCommandList(
	DXDevice* const Device, 
	CommandListDesc CommandListDesc, 
	DXCommandAllocator * CommandAllocator, 
	DXPipelineState * PipelineState)
{
	DXGraphicsCommandList * commandList = nullptr;

	ThrowIfFailed(Device->CreateCommandList(CommandListDesc.NodeMask, CommandListDesc.Type, CommandAllocator, PipelineState, IID_PPV_ARGS(&commandList)));

	return commandList;
}

DXGraphicsCommandList1* Utils::CreateGraphicsCommandList1(
	DXDevice* const Device, 
	CommandListDesc CommandListDesc)
{
	DXGraphicsCommandList1 * commandList1	= nullptr;
	DXDevice4* device4						= nullptr;	//Use Device4 to create a command list with closed state

	Device->QueryInterface(IID_PPV_ARGS(&device4));	//Make device4 from device by quering the interface
	ThrowIfFailed(device4->CreateCommandList1(CommandListDesc.NodeMask, CommandListDesc.Type, CommandListDesc.Flags, IID_PPV_ARGS(&commandList1)));

	return commandList1;
}

DXCommandQueue * Utils::CreateCommandQueue(DXDevice * const Device, D3D12_COMMAND_QUEUE_DESC CommandQueueDesc)
{
	DXCommandQueue * CommandQueue = nullptr;
	ThrowIfFailed(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue)));
	return CommandQueue;
}
