
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include "D3D12Helpers.h"

// Class encapsulated RAII idiom
// Sets barrier before -> after in the constructor
// Resets barrier after -> before in the destructor
class ResourceBarrier
{
public:

	ResourceBarrier() = delete;
	~ResourceBarrier();

	ResourceBarrier(
		DXResource*const resource,
		DXGraphicsCommandList*const commandList,
		D3D12_RESOURCE_STATES stateBefore, 
		D3D12_RESOURCE_STATES stateAfter);

	// Utility functions


private:

	DXResource*					resource;
	DXGraphicsCommandList*		commandList;
	D3D12_RESOURCE_STATES		stateBefore;
	D3D12_RESOURCE_STATES		stateAfter;
};