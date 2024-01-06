
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <windows.h>
#include <dxgi1_6.h>

#include "d3dx12.h"
#include "D3D12Helpers.h"
#include "D3D12Utils.h"

//Forward Declarations
struct alignas(256) SceneConstantBuffer;

//Usings
using DirectX::XMFLOAT4X4;
using DirectX::XMFLOAT4;

class D3D123DApp
{
public:
	D3D123DApp() = delete;
	~D3D123DApp() = default; //just do nothing, app takes care of deallocation with smart pointers

	D3D123DApp(UINT windowWidth, UINT windowHeight, std::wstring windowName);

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
	ComPtr<ID3D12DescriptorHeap>						rtvHeap;
	std::vector<ComPtr<DXResource>>						renderTargets{};
	UINT												rtvIncrementDescriptorSize;

	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>							pipelineState;
	ComPtr<ID3D12RootSignature>							rootSignature;

	// D3D12 Vertex data
	ComPtr<DXResource>									vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							vertexBufferView;

	// D3D12 Index buffer data
	ComPtr<DXResource>									indexBuffer;
	D3D12_INDEX_BUFFER_VIEW								indexBufferView;

	// D3D12 Depth Stencil
	ComPtr<ID3D12DescriptorHeap>						dsvHeap;
	ComPtr<DXResource>									depthStencil;
	UINT												dsvIncrementDescriptorSize;

	// Own util class
	UINT												constantBufferPerObjectAlignedSize = (sizeof(SceneConstantBuffer) + 255) & ~255;
	SceneConstantBuffer									cbPerObject; // this is the constant buffer data we will send to the gpu 
	std::vector<ComPtr<DXResource>>						cbUploadHeaps; // this is the memory on the gpu where constant buffers for each frame will be placed
	std::vector<UINT8*>									cbvGPUAddress; // this is a pointer to each of the constant buffer resource heaps

	XMFLOAT4X4											cameraProjMat; // this will store our projection matrix
	XMFLOAT4X4											cameraViewMat; // this will store our view matrix

	XMFLOAT4											cameraPosition; // this is our cameras position vector
	XMFLOAT4											cameraTarget; // a vector describing the point in space our camera is looking at
	XMFLOAT4											cameraUp; // the worlds up vector

	XMFLOAT4X4											cube1WorldMat; // our first cubes world matrix (transformation matrix)
	XMFLOAT4X4											cube1RotMat; // this will keep track of our rotation for the first cube
	XMFLOAT4											cube1Position; // our first cubes position in space

	XMFLOAT4X4											cube2WorldMat; // our first cubes world matrix (transformation matrix)
	XMFLOAT4X4											cube2RotMat; // this will keep track of our rotation for the second cube
	XMFLOAT4											cube2PositionOffset; // our second cube will rotate around the first cube, so this is the position offset from the first cube

	UINT												numCubeIndices; // the number of indices to draw the cube
};
