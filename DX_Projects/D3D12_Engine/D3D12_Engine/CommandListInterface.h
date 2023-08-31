
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include "D3D12Helpers.h"

class CommandListInterface
{
public:

	CommandListInterface() = default;
	virtual ~CommandListInterface() = default;

	CommandListInterface(
		DXDevice* const device, 
		const D3D12_COMMAND_LIST_TYPE commandListType, 
		_In_opt_ DXCommandAllocator * const commandAllocator = nullptr);

	// Utility functions
	void ResetCommandAllocator() const;
	[[nodiscard]] DXCommandAllocator* GetCommandAllocator() const { return commandAllocator.Get(); }

private:

	// Dummy function to make a interface from this class
	virtual void IAmInterface() = 0;

	ComPtr<DXCommandAllocator>				commandAllocator;
};