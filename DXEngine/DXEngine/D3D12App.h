
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_APP_H_INCLUDED
#define D3D12_APP_H_INCLUDED


#include "pch.h"
#include "D3D12Utils.h"
#include "D3D12Math.h"

//struct alignas(256) ConstantBufferPerObject;
//struct alignas(256) CameraMatrices;
//struct alignas(256) CubeMatrices;


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
	ComPtr<DXGraphicsCommandList>						commandList;
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
	std::vector<ComPtr<ID3D12Resource>>					renderTargets{};
	UINT												rtvIncrementDescriptorSize;

	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>							pipelineState;
	ComPtr<ID3D12RootSignature>							rootSignature;

	// D3D12 Vertex data
	ComPtr<ID3D12Resource>								vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							vertexBufferView;

	// D3D12 Index buffer data
	ComPtr<ID3D12Resource>								indexBuffer;
	D3D12_INDEX_BUFFER_VIEW								indexBufferView;

	// D3D12 Depth Stencil
	ComPtr<ID3D12DescriptorHeap>						dsvHeap;
	ComPtr<ID3D12Resource>								depthStencil;
	UINT												dsvIncrementDescriptorSize;

	// D3D12 Constant buffer data
	std::vector<ComPtr<ID3D12Resource>>					constantBufferUploadHeaps;	// this is the memory on the gpu where constant buffers for each frame will be placed
	std::vector<UINT8>									cbvGPUAddress;				// this is a pointer to each of the constant buffer resource heaps
	ConstantBufferPerObject								cbvPerCube;
	CameraMatrices										camMatrices;
	CubeMatrices										cube1Matrices;
	CubeMatrices										cube2Matrices;

	// Own util class

};

#endif // D3D12_APP_H_INCLUDED