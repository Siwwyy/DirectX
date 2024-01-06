#ifndef D3D12_UTILS_H_INCLUDED
#define D3D12_UTILS_H_INCLUDED
#pragma once

#include <DirectXMath.h>
#include "D3D12Helpers.h"

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
	[[nodiscard]] DXGraphicsCommandList *			CreateGraphicsCommandList(DXDevice * const device, CommandListDesc commandListDesc, DXCommandAllocator * commandAllocator, DXPipelineState * pipelineState);

	// Creates a GraphicsCommandList1 in closed state, using Device4 inside
	[[nodiscard]] DXGraphicsCommandList1 *			CreateGraphicsCommandList1(DXDevice * const device, CommandListDesc commandListDesc);

}

#endif //D3D12_UTILS_H_INCLUDED