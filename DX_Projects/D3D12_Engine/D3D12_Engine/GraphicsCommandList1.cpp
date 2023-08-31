#include "GraphicsCommandList1.h"


using Helpers::ThrowIfFailed;

GraphicsCommandList1::GraphicsCommandList1(
	DXDevice* const device,
	const D3D12_COMMAND_LIST_TYPE commandListType,
	_In_opt_ DXPipelineState * const pipelineState,
	_In_opt_ DXCommandAllocator * const commandAllocator)
	: CommandListInterface(device, commandListType, commandAllocator)
	, pipelineState(pipelineState)
{
	DXASSERT(device, "Device pointer must be provided");

	// Create the command list.
	ThrowIfFailed(device->CreateCommandList(0, commandListType, GetCommandAllocator(), pipelineState, IID_PPV_ARGS(&commandList)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(commandList->Close());


	commandList->SetName(L"COMMAND LIST");
	if(pipelineState)
	{
		this->pipelineState->SetName(L"PIPELINE STATE");
	}
}

void GraphicsCommandList1::ResetCommandList(_In_opt_ DXPipelineState * const pipelineState)
{
	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be, before 
	// re-recording.
	if(pipelineState)
	{
		this->pipelineState = ComPtr<DXPipelineState>(pipelineState);
		this->pipelineState->SetName(L"PIPELINE STATE");
		//ThrowIfFailed(this->pipelineState->QueryInterface(IID_PPV_ARGS(&pipelineState)));
	}
	
	ThrowIfFailed(commandList->Reset(GetCommandAllocator(), this->pipelineState.Get()));
}

void GraphicsCommandList1::CloseCommandList() const
{
	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(commandList->Close());
}
