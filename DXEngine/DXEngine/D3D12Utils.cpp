#include "D3D12Utils.h"


DXGraphicsCommandList* Utils::CreateGraphicsCommandList(
	DXDevice* const device, 
	CommandListDesc commandListDesc, 
	DXCommandAllocator * commandAllocator, 
	DXPipelineState * pipelineState)
{
	DXGraphicsCommandList * commandList = nullptr;

	device->CreateCommandList(commandListDesc.nodeMask, commandListDesc.Type, commandAllocator, pipelineState, IID_PPV_ARGS(&commandList));

	return commandList;
}

DXGraphicsCommandList1* Utils::CreateGraphicsCommandList1(
	DXDevice* const device, 
	CommandListDesc commandListDesc)
{
	DXGraphicsCommandList1 * commandList1	= nullptr;
	DXDevice4* device4						= nullptr;	//Use Device4 to create a command list with closed state

	device->QueryInterface(IID_PPV_ARGS(&device4));	//Make device4 from device by quering the interface
	device4->CreateCommandList1(commandListDesc.nodeMask, commandListDesc.Type, commandListDesc.Flags, IID_PPV_ARGS(&commandList1));

	return commandList1;
}