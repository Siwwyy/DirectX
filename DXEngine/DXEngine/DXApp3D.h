
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <windows.h>
#include <dxgi1_6.h>

#include "d3dx12.h"
#include "D3D12Helpers.h"
#include "D3D12Utils.h"

class DXApp3D
{
public:

	DXApp3D() = delete;
	~DXApp3D() = default; //just do nothing, app takes care of deallocation with smart pointers

	DXApp3D(UINT windowWidth, UINT windowHeight, std::wstring windowName);

	// Getters
	[[nodiscard]] UINT											GetWindowWidth() const { return windowWidth; }
	[[nodiscard]] UINT											GetWindowHeight() const { return windowHeight; }
	[[nodiscard]] float											GetAspectRatio() const { return aspectRatio; }
	[[nodiscard]] const std::wstring&							GetWindowName() const { return windowName; }

	// Functions
	void Initialize();
	void Render();
	void Update();
	void Destroy();

private:

	//Utility functions
	void PopulateCommandLists();
	void WaitForPreviousFrame();

	// Window Properties
	UINT												windowWidth;
	UINT												windowHeight;
	float												aspectRatio;
	std::wstring										windowName;

	// D3D12 Window Properties
	CD3DX12_VIEWPORT									viewPort;
	CD3DX12_RECT										scissorRect;

	// D3D12 Variables
	ComPtr<DXFactory4>									factory;
	ComPtr<DXDevice>									device;
	ComPtr<DXCommandQueue>								commandQueue;
	ComPtr<DXGraphicsCommandList1>						commandList;
	ComPtr<DXCommandAllocator>							commandAllocator;

	// D3D12 Synchronization CPU<->GPU
	ComPtr<DXFence>										fence;
	UINT64												fenceValue;
	HANDLE												fenceEvent;

	// D3D12 SwapChain
	ComPtr<IDXGISwapChain3>								swapChain;
	UINT												bufferCount;
	UINT												currentFrameIdx;

	// D3D12 Frame Buffer Render Target
	ComPtr<DXDescriptorHeap>							rtvHeap;
	std::vector<ComPtr<DXResource>>						renderTargets{};
	UINT												rtvIncrementDescriptorSize;

};