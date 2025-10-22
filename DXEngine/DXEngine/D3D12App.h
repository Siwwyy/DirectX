
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
	void Update(float DeltaTime);
	void Destroy();


private:

	//Utility functions
	void InitializePerFrameResources();
	void BeginFrame();
	void EndFrame();
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

	// D3D12 Vertex data
	ComPtr<ID3D12Resource>								VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							VertexBufferView;

	// D3D12 Index buffer data
	ComPtr<ID3D12Resource>								IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW								IndexBufferView;

	// D3D12 Depth Stencil
	ComPtr<ID3D12DescriptorHeap>						DsvHeap;
	ComPtr<ID3D12Resource>								DepthStencil;
	UINT												DsvIncrementDescriptorSize;

	// D3D12 Constant buffer data
	std::vector<ComPtr<ID3D12Resource>>					ConstantBufferUploadHeaps;	// this is the memory on the gpu where constant buffers for each frame will be placed
	std::vector<UINT8*>									CbvGPUAddress;				// this is a pointer to each of the constant buffer resource heaps
	ConstantBufferPerObject								CbvPerCube;
	CameraMatrices										CamMatrices;
	CubeMatrices										Cube1Matrices;
	CubeMatrices										Cube2Matrices;

	// Own util class
	ComPtr<ID3DBlob>									VertexShader;				// Vertex shader blob
	ComPtr<ID3DBlob>									PixelShader;				// Pixel shader blob
	ComPtr<ID3D12Resource>								VertexBufferUploadHeap;
	ComPtr<ID3D12Resource>								IndexBufferUploadHeap;


	// Texture loading stuffs
	ID3D12Resource* textureBuffer; // the resource heap containing our texture

	int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int &bytesPerRow);

	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

	ID3D12DescriptorHeap* mainDescriptorHeap;
	ID3D12Resource* textureBufferUploadHeap;

	// Font stuffs
	ID3D12PipelineState* textPSO; // pso containing a pipeline state

	Font arialFont; // this will store our arial font information

	int maxNumTextCharacters = 1024; // the maximum number of characters you can render during a frame. This is just used to make sure
	// there is enough memory allocated for the text vertex buffer each frame

	std::vector<ComPtr<ID3D12Resource>>			textVertexBuffer;
	std::vector<D3D12_VERTEX_BUFFER_VIEW>		textVertexBufferView;	// a view for our text vertex buffer
	std::vector<UINT8>							textVBGPUAddress;		// this is a pointer to each of the text constant buffers

	Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight); // load a font

	void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
};

#endif // D3D12_APP_H_INCLUDED