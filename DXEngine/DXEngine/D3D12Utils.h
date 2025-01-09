
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_UTILS_H_INCLUDED
#define D3D12_UTILS_H_INCLUDED

#include "pch.h"

namespace Utils
{
	/***************************
	 **** CLASSES & STRUCTS ****
	 ***************************/

	/***************************
	 ******** FUNCTIONS ********
	 ***************************/

	using Helpers::CommandListDesc;

	// Creates a Command List. Optionally with Command Allocator and Pipeline State
	[[nodiscard]] DXGraphicsCommandList *			CreateGraphicsCommandList(DXDevice * const Device, CommandListDesc CommandListDesc, DXCommandAllocator * CommandAllocator, DXPipelineState * PipelineState = nullptr);

	// Creates a GraphicsCommandList1 in closed state, using Device4 inside
	[[nodiscard]] DXGraphicsCommandList1 *			CreateGraphicsCommandList1(DXDevice * const Device, CommandListDesc CommandListDesc);

	// Creates a Command Queue
	[[nodiscard]] DXCommandQueue *					CreateCommandQueue(DXDevice * const Device, D3D12_COMMAND_QUEUE_DESC CommandQueueDesc);

}

#endif //D3D12_UTILS_H_INCLUDED