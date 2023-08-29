
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <windows.h>
#include <dxgi1_6.h>


#include "CommandQueue.h"
#include "d3dx12.h"
#include "D3D12Helpers.h"
#include "GraphicsCommandList1.h"

class D3D12App
{
public:
	D3D12App() = delete;
	~D3D12App() = default; //just do nothing, app takes care of deallocation with smart pointers

	D3D12App(UINT windowWidth, UINT windowHeight, std::wstring windowName);

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

	void PopulateCommandLists();

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

	// D3D12 Frame Buffer Render Target
	ComPtr<ID3D12DescriptorHeap>						rtvHeap;
	std::vector<ComPtr<ID3D12Resource>>					renderTargets{};
	UINT												rtvIncrementDescriptorSize;

	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>							pipelineState;
	ComPtr<ID3D12RootSignature>							rootSignature;

	// D3D12 Vertex data
	ComPtr<ID3D12Resource>								vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							vertexBufferView;

	// Own util class
	std::unique_ptr<CommandQueue>						commandQueue;
	std::unique_ptr<GraphicsCommandList1>				commandList;

};
