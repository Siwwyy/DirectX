
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_UTILS_H_INCLUDED
#define D3D12_UTILS_H_INCLUDED

#include "pch.h"

namespace Utils
{
	using Helpers::CommandListDesc;
	using Helpers::RootParamHelper;

	/***************************
	 **** CLASSES & STRUCTS ****
	 ***************************/

	/***************************
	 ******** FUNCTIONS ********
	 ***************************/



	// Creates a Command List. Optionally with Command Allocator and Pipeline State
	[[nodiscard]] DXGraphicsCommandList *				CreateGraphicsCommandList(DXDevice * const Device, CommandListDesc CommandListDesc, DXCommandAllocator * CommandAllocator, DXPipelineState * PipelineState = nullptr);

	// Creates a GraphicsCommandList1 in closed state, using Device4 inside
	[[nodiscard]] DXGraphicsCommandList1 *				CreateGraphicsCommandList1(DXDevice * const Device, CommandListDesc CommandListDesc);

	// Creates a Command Queue
	[[nodiscard]] DXCommandQueue *						CreateCommandQueue(DXDevice * const Device, D3D12_COMMAND_QUEUE_DESC CommandQueueDesc);

	// Creates root constant
	[[nodiscard]] D3D12_ROOT_CONSTANTS					CreateRootConstant(UINT RegisterSpace, UINT ShaderRegister, UINT Num32BitValues);

	// Creates root descriptor
	[[nodiscard]] D3D12_ROOT_DESCRIPTOR					CreateRootDescriptor(UINT RegisterSpace, UINT ShaderRegister);

	// Creates root descriptor
	[[nodiscard]] D3D12_ROOT_DESCRIPTOR_TABLE			CreateRootDescriptorTable(UINT NumDescriptorRanges, const D3D12_DESCRIPTOR_RANGE * DescriptorRange);

	// Creates root parameter/s
	[[nodiscard]] std::vector<D3D12_ROOT_PARAMETER>		CreateRootParameters(std::initializer_list<RootParamHelper> Params);

}

#endif //D3D12_UTILS_H_INCLUDED