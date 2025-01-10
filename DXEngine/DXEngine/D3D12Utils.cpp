
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Utils.h"

using Helpers::ThrowIfFailed;
using Helpers::RootParamHelper;

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

D3D12_ROOT_CONSTANTS Utils::CreateRootConstant(UINT RegisterSpace, UINT ShaderRegister, UINT Num32BitValues)
{
	D3D12_ROOT_CONSTANTS temp	= {};
	temp.RegisterSpace			= RegisterSpace;
	temp.ShaderRegister			= ShaderRegister;
	temp.Num32BitValues			= Num32BitValues;
	return temp; //RVO, no move needed (Probably)
}

D3D12_ROOT_DESCRIPTOR Utils::CreateRootDescriptor(UINT RegisterSpace, UINT ShaderRegister)
{
	D3D12_ROOT_DESCRIPTOR temp	= {};
	temp.RegisterSpace			= RegisterSpace;
	temp.ShaderRegister			= ShaderRegister;
	return temp; //RVO, no move needed (Probably)
}

D3D12_ROOT_DESCRIPTOR_TABLE Utils::CreateRootDescriptorTable(UINT NumDescriptorRanges, const D3D12_DESCRIPTOR_RANGE * DescriptorRange)
{
	D3D12_ROOT_DESCRIPTOR_TABLE temp	= {};
	temp.NumDescriptorRanges			= NumDescriptorRanges;
	temp.pDescriptorRanges				= DescriptorRange;
	return temp; //RVO, no move needed (Probably)
}

std::vector<D3D12_ROOT_PARAMETER> Utils::CreateRootParameters(std::initializer_list<RootParamHelper> Params)
{
	std::vector<D3D12_ROOT_PARAMETER> temp;
	for (const Helpers::RootParamHelper& param : Params)
	{
		temp.emplace_back(param.CreateRootParameter());
	}
	return temp; //initializer list<D3D12_ROOT_PARAMETER>
}
