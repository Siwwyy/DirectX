#include "ResourceBarrier.h"

ResourceBarrier::~ResourceBarrier()
{
	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, stateAfter, stateBefore);
	commandList->ResourceBarrier(1, &barrier);
}

ResourceBarrier::ResourceBarrier(
	DXResource* const resource,
	DXGraphicsCommandList*const commandList,
	D3D12_RESOURCE_STATES stateBefore,
	D3D12_RESOURCE_STATES stateAfter)
	: resource(resource)
	, commandList(commandList)
	, stateBefore(stateBefore)
	, stateAfter(stateAfter)
{
	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, stateBefore, stateAfter);
	commandList->ResourceBarrier(1, &barrier);
}
