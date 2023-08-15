
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <windows.h>
#include <dxgi1_6.h>


#include "d3dx12.h"
#include "D3D12Helpers.h"

class D3D12App
{
public:
	D3D12App() = delete;
	~D3D12App() {} //just do nothing, app takes care of deallocation with smart pointers
	//~D3D12App();

	D3D12App(UINT windowWidth, UINT windowHeight, std::wstring windowName);

	// Getters
	UINT											GetWindowWidth() const { return windowWidth; }
	UINT											GetWindowHeight() const { return windowHeight; }
	float											GetAspectRatio() const { return aspectRatio; }
	const std::wstring&								GetWindowName() const { return windowName; }

	// Functions
	void Initialize();
	void Render();
	void Update();
	void Destroy();

private:

	//Utility functions
	//void WaitForPreviousFrame();

	// Window Properties
	UINT												windowWidth;
	UINT												windowHeight;
	float												aspectRatio;
	std::wstring										windowName;

	// D3D12 Window Properties
	CD3DX12_VIEWPORT									viewPort;
	CD3DX12_RECT										scissorRect;

	// D3D12 Variables
	ComPtr<IDXGIFactory4>								factory;
	ComPtr<ID3D12Device>								device;

	// D3D12 SwapChain
	ComPtr<IDXGISwapChain3>								swapChain;
	UINT												bufferCount;
	UINT												currentFrameIdx;

};