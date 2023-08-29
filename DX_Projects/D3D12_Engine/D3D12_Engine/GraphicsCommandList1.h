
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include "CommandListInterface.h"

class GraphicsCommandList1 : public CommandListInterface
{
public:

	DISABLE_COPY(GraphicsCommandList1)

	GraphicsCommandList1() = default;
	virtual ~GraphicsCommandList1() override = default;

	GraphicsCommandList1(
		DXDevice* const device,
		const D3D12_COMMAND_LIST_TYPE commandListType,
		_In_opt_ DXPipelineState * const pipelineState = nullptr,
		_In_opt_ DXCommandAllocator * const commandAllocator = nullptr);

	// Utility functions

	// If pipeline is provided, then command list will be reseted with new pipeline State,
	// otherwise, pipeline State will be nullptr or based on whats was provided in constructor
	void ResetCommandList(_In_opt_ DXPipelineState * const pipelineState = nullptr);
	void CloseCommandList() const;

	// Functor for easier recording and getting pointer to command list
	[[nodiscard]] DXGraphicsCommandList1* GetCommandList() const noexcept { return commandList.Get(); }

private:

	// Dummy function
	void IAmInterface() override {}

	ComPtr<DXGraphicsCommandList1>			commandList;
	ComPtr<DXPipelineState>					pipelineState;
};
