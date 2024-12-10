
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_APP_H_INCLUDED
#define D3D12_APP_H_INCLUDED

#include <DirectXMath.h>
#include <wrl/client.h>
#include <windows.h>

#include "D3D12Helpers.h"
#include "D3D12Utils.h"


struct alignas(256) SceneConstantBuffer
{
	DirectX::XMFLOAT4 positionMultiplier;
	DirectX::XMFLOAT4 colorMultiplier;
};
static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

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

	void ArrowUp();
	void ArrowDown();

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
	ComPtr<ID3D12DescriptorHeap>						cbvHeap;
	ComPtr<ID3D12Resource>								constantBuffer;
	SceneConstantBuffer									constantBufferData;
	UINT8*												constantBufferDataGPUAddress;

	// Own util class

};

#endif // D3D12_APP_H_INCLUDED