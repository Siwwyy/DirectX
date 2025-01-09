
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_APP_H_INCLUDED
#define D3D12_APP_H_INCLUDED


#include "pch.h"
#include "D3D12Utils.h"
#include "D3D12Math.h"

void Initialize_Fence();
void Initialize_CommandQueue();
void Initialize_CommandAllocator();
void Initialize_Fences();
void Initialize_Fences();

class D3D12App
{
public:
	D3D12App() = delete;
	~D3D12App() = default; //just do nothing, app takes care of deallocation with smart pointers

	D3D12App(UINT windowWidth, UINT windowHeight, std::wstring windowName);

	// Getters
	[[nodiscard]] UINT											GetWindowWidth() const { return WindowWidth; }
	[[nodiscard]] UINT											GetWindowHeight() const { return WindowHeight; }
	[[nodiscard]] float											GetAspectRatio() const { return AspectRatio; }
	[[nodiscard]] const std::wstring&							GetWindowName() const { return WindowName; }

	// Functions
	void Initialize();
	void Render();
	void Update();
	void Destroy();


private:

	//Utility functions
	void InitializePerFrameResources();
	void PopulateCommandLists();
	void SubmitCommandLists();
	void WaitForPreviousFrame();

	// Window Properties
	UINT												WindowWidth;
	UINT												WindowHeight;
	float												AspectRatio;
	std::wstring										WindowName;

	// D3D12 Window Properties
	CD3DX12_VIEWPORT									ViewPort;
	CD3DX12_RECT										ScissorRect;

	// D3D12 Variables
	ComPtr<DXFactory4>									Factory;
	ComPtr<DXDevice>									Device;
	ComPtr<DXCommandQueue>								CommandQueue;
	ComPtr<DXGraphicsCommandList>						CommandList;
	std::vector<ComPtr<DXCommandAllocator>>				CommandAllocators;

	// D3D12 Synchronization CPU<->GPU
	std::vector<ComPtr<DXFence>>						Fences;
	std::vector<UINT64>									FenceValue;
	HANDLE												FenceEvent;

	// D3D12 SwapChain
	ComPtr<IDXGISwapChain3>								SwapChain;
	UINT												CurrentFrameIdx;

	// D3D12 Frame Buffer Render Target
	ComPtr<ID3D12DescriptorHeap>						RtvHeap;
	std::vector<ComPtr<ID3D12Resource>>					RenderTargets{};
	UINT												RtvIncrementDescriptorSize;

	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>							PipelineState;
	ComPtr<ID3D12RootSignature>							RootSignature;

	//// D3D12 Vertex data
	//ComPtr<ID3D12Resource>								vertexBuffer;
	//D3D12_VERTEX_BUFFER_VIEW							vertexBufferView;

	//// D3D12 Index buffer data
	//ComPtr<ID3D12Resource>								indexBuffer;
	//D3D12_INDEX_BUFFER_VIEW								indexBufferView;

	//// D3D12 Depth Stencil
	//ComPtr<ID3D12DescriptorHeap>						dsvHeap;
	//ComPtr<ID3D12Resource>								depthStencil;
	//UINT												dsvIncrementDescriptorSize;

	//// D3D12 Constant buffer data
	//std::vector<ComPtr<ID3D12Resource>>					constantBufferUploadHeaps;	// this is the memory on the gpu where constant buffers for each frame will be placed
	//std::vector<UINT8*>									cbvGPUAddress;				// this is a pointer to each of the constant buffer resource heaps
	//ConstantBufferPerObject								cbvPerCube;
	//CameraMatrices										camMatrices;
	//CubeMatrices										cube1Matrices;
	//CubeMatrices										cube2Matrices;

	// Own util class

};

#endif // D3D12_APP_H_INCLUDED