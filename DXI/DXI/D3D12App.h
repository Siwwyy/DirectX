//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_APP_H_INCLUDED
#define D3D12_APP_H_INCLUDED


#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Texture.h"

// Objects
#include "Objects/Camera.h"
#include "Objects/Plane.h"
#include "Objects/Cube.h"


//APP
class D3D12App
{
public:

	D3D12App()	= delete;
	// Ctors & DCtors
	D3D12App(UINT windowWidth, UINT windowHeight, std::wstring windowName);
	~D3D12App() = default; //just do nothing, app takes care of deallocation with smart pointers
	

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


	// SoftwareRasterizer
	void InitializeSoftwareRasterizer();
	void RenderSoftwareRasterizer();


	struct SoftwareRasterizer
	{
		SoftwareRasterizer(const UINT Width, const UINT Height) : Width(Width), Height(Height) {}


		//Utility functions
		void InitializeResources(ComPtr<DXDevice>& Device);
		void InitalizeShader(D3D12ShaderCompiler& ShaderCompiler);
		void InitializePSO(ComPtr<DXDevice>& Device);
		void InitializeData(ComPtr<DXDevice>& Device, const Camera& AppCamera, ComPtr<DXGraphicsCommandList>& CommandList);
		void BeginCompute();
		void Compute();
		void EndCompute();
		void SubmitCompute();

		void CopyFromRenderTarget(ComPtr<DXDevice>& Device, const ComPtr<DXResource>& RT, ComPtr<DXGraphicsCommandList>& CommandList);
		void CopyToRenderTarget(ComPtr<DXDevice>& Device, ComPtr<DXResource>& RT, ComPtr<DXGraphicsCommandList>& CommandList);
		void CopyVertexBufferPos(ComPtr<DXDevice>& Device, Helpers::VERTEX_HELPER& VertexUploadToGPU, ComPtr<DXGraphicsCommandList>& CommandList);
		void CopyVertexBufferColor(ComPtr<DXDevice>& Device, Helpers::VERTEX_HELPER& VertexUploadToGPU2, ComPtr<DXGraphicsCommandList>& CommandList);
		void CopyIndexBuffer(ComPtr<DXDevice>& Device, Helpers::INDEX_HELPER& IndexUploadToGPU, ComPtr<DXGraphicsCommandList>& CommandList);

		//
		UINT Width;
		UINT Height;

		//
		ComPtr<DXCommandQueue>								CommandQueue;
		ComPtr<DXGraphicsCommandList>						CommandList;
		ComPtr<DXCommandAllocator>							CommandAllocator;

		// D3D12 Synchronization CPU<->GPU
		ComPtr<DXFence>										Fence;
		UINT64												FenceValue;
		HANDLE												FenceEvent;

		// Pipeline state and root signature
		ComPtr<ID3D12PipelineState>							PipelineState;
		ComPtr<ID3D12RootSignature>							RootSignature;
		ComPtr<ID3DBlob>									ComputeShader;				// Compute shader blob


		// Constant Buffers
		ComPtr<DXResource>									ConstantBufferUpload;	// this is the memory on the gpu where constant buffers for each frame will be placed
		UINT8*												CbvGPUAddress;				// this is a pointer to each of the constant buffer resource heaps
		ConstantBufferSoftwareRasterizer					CbvSoftwareRasterizer;
		UINT												IncrementSizeCBVSRVUAV;


		// Descriptor Heap for SRV/UAV
		ComPtr<DXDescriptorHeap>							DescriptorHeap;
		UINT												DescriptorHeapIncrementSize;

		// 2 SRV + 1 UAV resources
		const static SIZE_T									SRVResourceCount = 5;
		const static SIZE_T									UAVResourceCount = 1;
		ComPtr<DXResource>									SRVResourcesUpload[SRVResourceCount];	// input color, input depth/stencil, input vertex pos, input vertex color
		ComPtr<DXResource>									SRVResources[SRVResourceCount];			// input color, input depth/stencil
		ComPtr<DXResource>									UAVResources[UAVResourceCount];			// output rasterized color

	};

	// Movement of camera
	void OnKeyDown(UINT8 key);
	void OnKeyUp(UINT8 key);

private:

	//Utility functions
	void InitializePerFrameResources();
	void InitalizeShaders();
	void InitializePSO();
	void BeginFrame();
	void EndFrame();
	void WaitForPreviousFrame();
	void FlushCommandList();

public:

	// SoftwareRasterizer
	SoftwareRasterizer SR;

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
	ComPtr<DXDescriptorHeap>							RtvHeap;
	std::vector<ComPtr<DXResource>>						RenderTargets;
	UINT												RtvIncrementDescriptorSize;

	// D3D12 Depth Stencil
	ComPtr<DXDescriptorHeap>							DsvHeap;
	ComPtr<DXResource>									DepthStencil;
	UINT												DsvIncrementDescriptorSize;

	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>							PipelineState;
	ComPtr<ID3D12RootSignature>							RootSignature;

	// D3D12 Vertex data
	ComPtr<DXResource>									VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							VertexBufferView;

	// D3D12 Index buffer data
	ComPtr<DXResource>									IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW								IndexBufferView;

	// Shaders
	D3D12ShaderCompiler									ShaderCompiler;
	ComPtr<ID3DBlob>									VertexShader;				// Vertex shader blob
	ComPtr<ID3DBlob>									PixelShader;				// Pixel shader blob

	// Constant Buffers
	std::vector<ComPtr<DXResource>>						ConstantBufferUploadHeaps;	// this is the memory on the gpu where constant buffers for each frame will be placed
	std::vector<UINT8*>									CbvGPUAddress;				// this is a pointer to each of the constant buffer resource heaps
	Plane												PlanePrimitive;
	Camera												Camera;
	ConstantBufferPerObject								CbvPerObject;



	//
	ComPtr<DXDescriptorHeap>	MainDescriptorHeap;
	UINT						IncrementDescriptorSize;

	// Texture class
	Texture MainTexture;
	Texture MeganeTexture;

	// Timer
	Helpers::StepTimer StepTimer;


	// NEW SECTION
	std::vector<std::unique_ptr<Primitive>>				Primitives;

	// CBV section for primitives
	UINT8*												PrimitivesConstantBufferPtr;
	ComPtr<DXResource>									PrimitiveCBV;	// constant buffer per primitive


	void StoreCBVDataForPrimitive(const Primitive& Primitive, const UINT PrimitiveIdx);

};

#endif // D3D12_APP_H_INCLUDED