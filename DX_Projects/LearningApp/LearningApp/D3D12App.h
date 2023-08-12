
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once
#include <wrl/client.h>

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

class D3D12App
{
public:
	D3D12App() = delete;
	~D3D12App() = default;

	D3D12App(UINT windowWidth, UINT windowHeight, std::wstring windowName);

	// Getters
	UINT										GetWindowWidth() const { return windowWidth; }
	UINT										GetWindowHeight() const { return windowHeight; }
	float										GetAspectRatio() const { return aspectRatio; }
	const std::wstring&							GetWindowName() const { return windowName; }

	// Functions
	void OnInit();
	void OnRender();
	void OnUpdate();
	void OnDestroy();

private:

	void InitializeFactory();
	void InitializeDevice();
	void InitializeCommandQueue();
	void InitializeCommandAllocator();
	void InitializeCommandList();
	void InitializeSwapChain();
	void InitializeFrameResources();
	void InitializeFence();

	ComPtr<ID3D12DescriptorHeap>				CreateRenderTargetDescriptorHeap(ComPtr<ID3D12Device> device, UINT& descriptorIncrementSize) const;
	static ComPtr<ID3D12Resource>				CreateRenderTargetResource(ComPtr<ID3D12Device> device, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptorHandle);

	//Utility functions
	void PopulateCommandLists();
	void WaitForPreviousFrame();

	ComPtr<IDXGIAdapter1>						GetAdapter(IDXGIFactory1* pFactory, bool useWarpAdapter = false, bool requestHighPerformanceAdapter = false) const;

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
	ComPtr<ID3D12CommandQueue>							commandQueue;
	ComPtr<ID3D12CommandAllocator>						commandAllocator;
	ComPtr<ID3D12GraphicsCommandList>					commandList;

	// D3D12 SwapChain
	ComPtr<IDXGISwapChain3>								swapChain;
	UINT												bufferCount;
	UINT												currentFrameIdx;

	// D3D12 Render Targets stuffs
	ComPtr<ID3D12DescriptorHeap>						rtvHeap;
	std::vector<ComPtr<ID3D12Resource>>					renderTargets{};
	UINT												rtvDescriptorSize;

	// D3D12 Fence CPU<->GPU Synchronization point
	HANDLE												fenceEvent;
	UINT64												fenceValue;
	ComPtr<ID3D12Fence>									fence;

};