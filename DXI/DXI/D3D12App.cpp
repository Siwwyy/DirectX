#include "D3D12App.h"


// Own Includes
#include "Win32Proc.h"
#include "D3D12Math.h"
#include "RenderGraph.h"

#include <string>
#include <pix3.h>

// Namespaces
using namespace Helpers;


//CONSTANTS
const D3D12_COMMAND_LIST_TYPE			COMMAND_LIST_TYPE			= D3D12_COMMAND_LIST_TYPE_DIRECT;
const D3D12_COMMAND_LIST_TYPE			COMMAND_LIST_TYPE_COMPUTE	= D3D12_COMMAND_LIST_TYPE_COMPUTE;
const UINT								BACK_BUFFER_COUNT			= 3;
const D3D_FEATURE_LEVEL					D3D12_FEATURE_LEVEL			= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
const DXGI_FORMAT						BACK_BUFFER_FORMAT			= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
const DXGI_FORMAT						DEPTH_STENCIL_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format
const DXGI_SAMPLE_DESC					SAMPLE_DESC					= SAMPLER_HELPER::CreateSampler(1, 0);

//////////////////////////////////////////////////////////////////


static VertexPosition VertexListComp[] =
{
	// front face
	{ -0.5f,  0.5f, -0.5f},
	{  0.5f, -0.5f, -0.5f},
	{ -0.5f, -0.5f, -0.5f},
	{  0.5f,  0.5f, -0.5f},

	// right side face
	{  0.5f, -0.5f, -0.5f},
	{  0.5f,  0.5f,  0.5f},
	{  0.5f, -0.5f,  0.5f},
	{  0.5f,  0.5f, -0.5f},

	// left side face
	{ -0.5f,  0.5f,  0.5f},
	{ -0.5f, -0.5f, -0.5f},
	{ -0.5f, -0.5f,  0.5f},
	{ -0.5f,  0.5f, -0.5f},

	// back face
	{  0.5f,  0.5f,  0.5f},
	{ -0.5f, -0.5f,  0.5f},
	{  0.5f, -0.5f,  0.5f},
	{ -0.5f,  0.5f,  0.5f},

	// top face
	{ -0.5f,  0.5f, -0.5f},
	{  0.5f,  0.5f,  0.5f},
	{  0.5f,  0.5f, -0.5f},
	{ -0.5f,  0.5f,  0.5f},

	// bottom face
	{  0.5f, -0.5f,  0.5f},
	{ -0.5f, -0.5f, -0.5f},
	{  0.5f, -0.5f, -0.5f},
	{ -0.5f, -0.5f,  0.5f},
};


// a quad (2 triangles)
static TypeOfIndice IndicesListComp[] =
{
	// front face
	0, 1, 2, // first triangle
	0, 3, 1, // second triangle

	// left face
	4, 5, 6, // first triangle
	4, 7, 5, // second triangle

	// right face
	8, 9, 10, // first triangle
	8, 11, 9, // second triangle

	// back face
	12, 13, 14, // first triangle
	12, 15, 13, // second triangle

	// top face
	16, 17, 18, // first triangle
	16, 19, 17, // second triangle

	// bottom face
	20, 21, 22, // first triangle
	20, 23, 21, // second triangle
};

using TypeOfVertexComp = VertexPosition;
static constexpr UINT CompVertexBufferSize = sizeof(VertexListComp);
static constexpr UINT CompIndexBufferSize = sizeof(IndicesListComp);
static constexpr UINT CompCubeNumIndices = CompIndexBufferSize / sizeof(TypeOfIndice);


D3D12_RESOURCE_STATES RTBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
D3D12_RESOURCE_STATES RTAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;

D3D12App::D3D12App(const UINT WindowWidth, const UINT WindowHeight, const std::wstring WindowName)
	: WindowWidth(WindowWidth)
	, WindowHeight(WindowHeight)
	, AspectRatio(static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight))
	, WindowName(WindowName)
	, ViewPort(0.f, 0.f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, 1.0f)
	, ScissorRect(0, 0, static_cast<LONG>(WindowWidth), static_cast<LONG>(WindowHeight))
	, CurrentFrameIdx(0)
	, RtvIncrementDescriptorSize(0)
	, Camera(WindowWidth, WindowHeight)
	, SR(WindowWidth, WindowHeight)
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");

	// Main ground plane
	PlanePrimitive.Transform({ 0.0f, 0.0f, 3.0f }, { -30.0f, 0.0f, 0.0f }, { 5.f, 5.f, 1.f });
}

void D3D12App::Initialize()
{
	//HMODULE module = LoadLibrary(L"WinPixEventRuntime.dll");
	// Initialize factory
	UINT DxgiFactoryFlags = 0;

#if DEBUG_MODE
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug3> DebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
		{
			DebugController->EnableDebugLayer();
			DebugController->SetEnableGPUBasedValidation(true);
			
			// Enable additional debug layers.
			DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}

	//Check support for SSE2
	DXASSERT(XMVerifyCPUSupport(), "Verify Support of DirectXMath!");

#endif // DEBUG_MODE

	//Create dxgi factory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

	// Initialize device
	const ComPtr<IDXGIAdapter1> HardwareAdapter = GetAdapter(Factory, D3D12_FEATURE_LEVEL);
	ThrowIfFailed(D3D12CreateDevice(
		HardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&Device)
	));

#if DEBUG_MODE
	{
		ComPtr<ID3D12InfoQueue> InfoQueue;
		ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,	true);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,		true);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,			true);
	}
#endif // DEBUG_MODE

	// Initialization of command queue
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask	= 0; //single GPU env for now
	CommandQueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type		= COMMAND_LIST_TYPE;
	CommandQueue				= Helpers::CreateCommandQueue(Device.Get(), CommandQueueDesc);

	// Initialization of Swap Chain
	// Get aplication window's handle (hwnd)
	auto WindowHwnd = Win32Proc::GetHwnd();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc		= {};
	SwapChainDesc.BufferCount				= BACK_BUFFER_COUNT;
	SwapChainDesc.Width						= WindowWidth;
	SwapChainDesc.Height					= WindowHeight;
	SwapChainDesc.Format					= BACK_BUFFER_FORMAT;
	SwapChainDesc.BufferUsage				= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.SwapEffect				= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//SwapChainDesc.SwapEffect				= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc.SampleDesc.Count			= 1;

	ComPtr<IDXGISwapChain1> SwapChain1;
	ThrowIfFailed(Factory->CreateSwapChainForHwnd(
		CommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		WindowHwnd,
		&SwapChainDesc,
		nullptr,
		nullptr,
		&SwapChain1
	));

	
	// Convert SwapChain1 interface to SwapChain4
	ThrowIfFailed(SwapChain1.As(&SwapChain)); // OR | auto SwapChain1 = dynamic_cast<IDXGISwapChain1*>(SwapChain.Get()); //retrieve IDXGISwapChain1 from IDXGISwapChain3

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(Factory->MakeWindowAssociation(WindowHwnd, DXGI_MWA_NO_ALT_ENTER));
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Create RTV Descriptor Heaps
	constexpr auto DescriptorHeapType	= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto DescriptorHeapFlags	= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc	= {};
	RtvHeapDesc.NumDescriptors				= BACK_BUFFER_COUNT;
	RtvHeapDesc.Type						= DescriptorHeapType;
	RtvHeapDesc.Flags						= DescriptorHeapFlags;
	ThrowIfFailed(Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	// Get Render Target View Increment Descriptor size
	RtvIncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(DescriptorHeapType);

	//Initialize resources per frame buffer
	InitializePerFrameResources();

	// Initialize Shaders
	InitalizeShaders();

	// Initialize PSO
	InitializePSO();

	// Create an event handle to use for frame synchronization.
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	//Initialization of command list
	{
		CommandList = Helpers::CreateGraphicsCommandList(
			Device.Get(),
			CommandListDesc{ 0, COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAG_NONE },
			CommandAllocators[0].Get(),
			PipelineState.Get());

		ThrowIfFailed(CommandList->Close()); //close not closed command list
		// Reset previously used command list and command allocator
		ThrowIfFailed(CommandList->Reset(CommandAllocators[0].Get(), PipelineState.Get()));
	}

	//Depth Stencil
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc	= {};
	DsvHeapDesc.NumDescriptors				= 1;
	DsvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DsvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&DsvHeap)));

	// create a depth stencil view
	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc	= {};
	DepthStencilViewDesc.Format							= DEPTH_STENCIL_FORMAT;
	DepthStencilViewDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
	DepthStencilViewDesc.Flags							= D3D12_DSV_FLAG_NONE;

	// clear value for depth stencil
	D3D12_CLEAR_VALUE DepthOptimizedClearValue		= {};
	DepthOptimizedClearValue.Format					= DEPTH_STENCIL_FORMAT;
	DepthOptimizedClearValue.DepthStencil.Depth		= 1.0f;
	DepthOptimizedClearValue.DepthStencil.Stencil	= 0;

	// Create Depth Stencil Texture 2D
	const auto DepthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DEPTH_STENCIL_FORMAT, 
		WindowWidth, 
		WindowHeight, 
		1, 0, 1, 0, 
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT,
		D3D12_HEAP_FLAG_NONE,
		&DepthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&DepthOptimizedClearValue,
		IID_PPV_ARGS(&DepthStencil)));

	// Create Depth Stencil View
	Device->CreateDepthStencilView(DepthStencil.Get(), &DepthStencilViewDesc, DsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Get Depth/Stencil View Descriptor Incremental Size (to get e.g., next resource/desc in the heap)
	DsvIncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	/////////

	// Test
	RenderGraph Graph(Device, CommandList);
	{
		// Init

		//// Create resources
		//// Vertex Buffer
		//auto VertexDescGPU		= BufferDesc::CreateBufferDesc(VertexBufferSize, 1, BufferType::VertexBuffer, DX_HEAP_PROPERTY_DEFAULT,	D3D12_RESOURCE_STATE_COMMON,		L"VertexBufferGPU");
		//auto VertexDescCPU		= BufferDesc::CreateBufferDesc(VertexBufferSize, 1, BufferType::VertexBuffer, DX_HEAP_PROPERTY_UPLOAD,	D3D12_RESOURCE_STATE_GENERIC_READ,	L"VertexBufferCPU");
		//auto VertexBufferGPU	= Graph.CreateBuffer(VertexDescGPU);
		//auto VertexBufferCPU	= Graph.CreateBuffer(VertexDescCPU);

		//// IndexBuffer
		//auto IndexDescGPU		= BufferDesc::CreateBufferDesc(IndexBufferSize, 1, BufferType::IndexBuffer, DX_HEAP_PROPERTY_DEFAULT,	D3D12_RESOURCE_STATE_COMMON,		L"IndexBufferGPU");
		//auto IndexDescCPU		= BufferDesc::CreateBufferDesc(IndexBufferSize, 1, BufferType::IndexBuffer, DX_HEAP_PROPERTY_UPLOAD,	D3D12_RESOURCE_STATE_GENERIC_READ,	L"IndexBufferCPU");
		//auto IndexBufferGPU		= Graph.CreateBuffer(IndexDescGPU);
		//auto IndexBufferCPU		= Graph.CreateBuffer(IndexDescCPU);

		//// AddPass
		//Graph.AddPass(TEXT("VertexSquare"), {&VertexBufferGPU, &VertexBufferCPU}, [=, &VertexBufferGPU, &VertexBufferCPU](DXDevice * Device, DXGraphicsCommandList * CommandList)
		//{
		//	/***************************
		//	****** VERTEX BUFFER ******
		//	***************************/
		//	constexpr const auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
		//	constexpr const auto StateAfter		= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		//	// Copy data to the intermediate upload heap and then schedule a copy 
		//	// from the upload heap to the vertex buffer.
		//	// store vertex buffer in upload heap
		//	D3D12_SUBRESOURCE_DATA	VertexData = {};
		//	VertexData.pData		= reinterpret_cast<UINT8*>(CubeVertices);
		//	VertexData.RowPitch		= VertexBufferSize;
		//	VertexData.SlicePitch	= VertexData.RowPitch;

		//	//PIXBeginEvent(CommandList.Get(), 0, L"Copy vertex buffer data to default resource...");

		//	// transition 
		//	RenderGraph::TransitionBarrier(CommandList, &VertexBufferGPU, StateBefore);

		//	// Update Subresource
		//	UpdateSubresources(CommandList, VertexBufferGPU.Resource.Get(), VertexBufferCPU.Resource.Get(), 0, 0, 1, &VertexData);

		//	// transition the vertex buffer data from copy destination state to vertex buffer state
		//	RenderGraph::TransitionBarrier(CommandList, &VertexBufferGPU, StateAfter);

		//	// Release the resources
		//	VertexBuffer		= VertexBufferGPU.Resource;
		//	VertexBufferView	= RenderGraph::CreateVertexBufferView(VertexBuffer->GetGPUVirtualAddress(), sizeof(TexVertex), VertexBufferSize);
		//});

		//// AddPass
		//Graph.AddPass(TEXT("IndexSquare"), { &IndexBufferGPU, &IndexBufferCPU }, [=, &IndexBufferGPU, &IndexBufferCPU](DXDevice * Device, DXGraphicsCommandList * CommandList)
		//{
		//	/***************************
		//	****** INDEX BUFFER ******
		//	***************************/
		//	constexpr const auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
		//	constexpr const auto StateAfter		= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		//	// Copy data to the intermediate upload heap and then schedule a copy 
		//	// from the upload heap to the vertex buffer.
		//	// store index buffer in upload heap
		//	D3D12_SUBRESOURCE_DATA	IndexData = {};
		//	IndexData.pData			= reinterpret_cast<UINT8*>(Indices);	// pointer to our index array
		//	IndexData.RowPitch		= IndexBufferSize;						 // size of all our index buffer
		//	IndexData.SlicePitch	= IndexData.RowPitch;					 // also the size of our index buffer

		//	//PIXBeginEvent(CommandList.Get(), 0, L"Copy vertex buffer data to default resource...");

		//	// transition 
		//	RenderGraph::TransitionBarrier(CommandList, &IndexBufferGPU, StateBefore);

		//	// Update Subresource
		//	UpdateSubresources(CommandList, IndexBufferGPU.Resource.Get(), IndexBufferCPU.Resource.Get(), 0, 0, 1, &IndexData);

		//	// transition the vertex buffer data from copy destination state to vertex buffer state
		//	RenderGraph::TransitionBarrier(CommandList, &IndexBufferGPU, StateAfter);

		//	// Release the resources
		//	IndexBuffer		= IndexBufferGPU.Resource;
		//	IndexBufferView = RenderGraph::CreateIndexBufferView(IndexBuffer->GetGPUVirtualAddress(), DXGI_FORMAT_R32_UINT, IndexBufferSize);
		//});

		/***************************
		 **** TEXTURING & Cube *****
		 ***************************/
		constexpr auto DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors = 2;
		HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HeapDesc.Type			= DescriptorHeapType;
		ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&MainDescriptorHeap)));

		// AddPass
		Graph.AddPass(TEXT("Cubes Init"), {}, [=](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			//Cube1.Init(Device,			CommandList);
			//Cube2.Init(Device,			CommandList);
			MainTexture.Load(Device,		CommandList,	L"bryanzar.png",	MainDescriptorHeap, IncrementDescriptorSize, 0);
			MeganeTexture.Load(Device,	CommandList,	L"megane.jpg",		MainDescriptorHeap, IncrementDescriptorSize, 1);
		});

		/***************************
		 ********** Plane **********
		 ***************************/
		Graph.AddPass(TEXT("Plane Init"), {}, [this](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			PlanePrimitive.Init(Device, CommandList);
		});
	}

	// NEW SECTION !!!

	// Create objects to render
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube1")));
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube2")));
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube3")));
	Primitives.emplace_back(std::make_unique<::Plane>(TEXT("Plane1")));

	/***************************
	 ******* Primitives ********
	 ***************************/
	Graph.AddPass(TEXT("ScenePrimitivesInit"), {}, [=](DXDevice * Device, DXGraphicsCommandList * CommandList)
	{
		// Constant buffer resource to store constant buffers for each primitive
		const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PrimitiveConstantBufferSize * Primitives.size());

		// Create a committed resource for cube constant buffer. This resource will be used to upload the constant buffer data to the GPU.
		ThrowIfFailed(Device->CreateCommittedResource(
			&DX_HEAP_PROPERTY_UPLOAD,
			D3D12_HEAP_FLAG_NONE,
			&UploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&PrimitiveCBV)));
		NAME_D3D12_OBJECT(PrimitiveCBV, L"PrimitiveCBV");

		// Read Range is the range of memory that we intend to read from the resource. We do not intend to read from this resource on the CPU, so we can set this to an empty range.
		CD3DX12_RANGE ReadRange(0, 0);

		// map the resource heap to get a gpu virtual address to the beginning of the heap
		ThrowIfFailed(PrimitiveCBV->Map(0, &ReadRange, reinterpret_cast<void**>(&PrimitivesConstantBufferPtr)));

		// Setup primitives and everything else related to the primitive itself
		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			// Init Vertex/Index Buffers
			Primitives[PrimitiveIdx]->Init(Device, CommandList);

			// Initial position, rotation, scale etc. for the primitive
			Primitives[PrimitiveIdx]->Transform({ -3.0f + 2 * static_cast<float>(PrimitiveIdx), 1.0f, 1.0f}, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
			
			// CBV per objects (for first frame)
			StoreCBVDataForPrimitive(*Primitives[PrimitiveIdx], PrimitiveIdx);
		}
	});

	// Execution
	Graph.Execute();

	// MUST BE LAST ONE HERE!!!
	{
		// Submit necessary things from command list
		// Execute command lists
		ThrowIfFailed(CommandList->Close()); //close command list for execution
		DXCommandList* CommandLists[] = { CommandList.Get() };
		CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

		// Move Fence / Wait for previous frame to end
		WaitForPreviousFrame();
	}
}

void D3D12App::Render()
{
	//Always BeginFrame first
	BeginFrame();

	// Drawing

	// Texture
	{
		// set the descriptor heap
		DXDescriptorHeap* DescriptorHeaps[] = { MainDescriptorHeap.Get()};
		CommandList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps);
	}

	// Plane
	PIXBeginEvent(CommandList.Get(), 0, L"Main Plane Rendering");
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		CommandList->SetGraphicsRootDescriptorTable(1, Handle);
		CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + (2 * ConstantBufferPerObjectSize));
		CommandList->IASetVertexBuffers(0, 1, &PlanePrimitive.VertexFactory.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		CommandList->IASetIndexBuffer(&PlanePrimitive.VertexFactory.IndexBufferView);
		CommandList->DrawIndexedInstanced(PlanePrimitive.GetNumIndices(), 1, 0, 0, 0); // draw plane
	}
	PIXEndEvent(CommandList.Get());

	// Primitives
	PIXBeginEvent(CommandList.Get(), 0, L"Primitives Rendering");
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		Handle.Offset(1, IncrementDescriptorSize);
		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			CommandList->SetGraphicsRootDescriptorTable(1, Handle);
			CommandList->SetGraphicsRootConstantBufferView(0, PrimitiveCBV->GetGPUVirtualAddress() + (PrimitiveIdx * PrimitiveConstantBufferSize));
			CommandList->IASetVertexBuffers(0, 1, &Primitives[PrimitiveIdx]->VertexFactory.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
			CommandList->IASetIndexBuffer(&Primitives[PrimitiveIdx]->VertexFactory.IndexBufferView);
			CommandList->DrawIndexedInstanced(Primitives[PrimitiveIdx]->GetNumIndices(), 1, 0, 0, 0); // draw primitive
		}
	}
	PIXEndEvent(CommandList.Get());


	{
		SR.CopyFromRenderTarget(Device, RenderTargets[CurrentFrameIdx], CommandList);
		SR.Compute();
	}

	//Always EndFrame last
	EndFrame();
}

void D3D12App::Update(float DeltaTime)
{
	// Other Updates
	StepTimer.Tick(NULL);
	if (CurrentFrameIdx % 509)
	{
		// Update window text with FPS value.
		wchar_t fps[64];
		swprintf_s(fps, L"%ufps", StepTimer.GetFramesPerSecond());
		//SetCustomWindowText(fps);
		std::wstring WindowText		= L"My Window | FPS: ";
		std::wstring WindowText2	= fps;
		SetWindowText(Win32Proc::GetHwnd(), (WindowText + WindowText2).c_str());
	}

	// Camera matrices
	Camera.Update(static_cast<float>(StepTimer.GetElapsedSeconds()));
	const auto ViewMat	= Camera.GetViewMatrix();
	const auto ProjMat	= Camera.GetProjMatrix();

	// update app logic, such as moving the camera or figuring out what objects are in view

	// store plane's world matrix
	{
		// create the wvp matrix and store in constant buffer
		const auto PlaneWorldMatrix				= PlanePrimitive.GetWorldMatrix();
		const auto PlaneScaleRotMatrix			= PlanePrimitive.GetWorldMatrixNoTranslation();
		XMMATRIX MVPMatPlane					= XMLoadFloat4x4(&PlaneWorldMatrix) * ViewMat * ProjMat;	// create wvp matrix
		XMMATRIX TransposedPlaneScaleRotMatrix	= XMMatrixTranspose(XMLoadFloat4x4(&PlaneScaleRotMatrix));		// must transpose wvp matrix for the gpu
		XMMATRIX TransposedMVPMatPlane			= XMMatrixTranspose(MVPMatPlane);							// must transpose wvp matrix for the gpu
		XMStoreFloat4x4(&CbvPerObject.LocalToWorld, TransposedPlaneScaleRotMatrix);							// store transposed world matrix in constant buffer
		XMStoreFloat4x4(&CbvPerObject.WorldToClip, TransposedMVPMatPlane);							// store transposed wvp matrix in constant buffer

		// copy our ConstantBuffer instance to the mapped constant buffer resource
		memcpy(CbvGPUAddress[CurrentFrameIdx] + (2 * ConstantBufferPerObjectSize), &CbvPerObject, sizeof(CbvPerObject));
	}

	// Primitives
	{
		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			StoreCBVDataForPrimitive(*Primitives[PrimitiveIdx], PrimitiveIdx);
		}
	}
}

void D3D12App::Destroy()
{
	// umap mapped memory for all primitive cbv
	PrimitiveCBV->Unmap(0, nullptr);

#if DEBUG_MODE
	{
		{
			ComPtr<ID3D12InfoQueue> InfoQueue;
			ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,	false);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,		false);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,			false);
		}

		ComPtr<ID3D12DebugDevice2> DebugDevice;
		ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&DebugDevice)));
		Device->Release();
		ThrowIfFailed(DebugDevice->ReportLiveDeviceObjects(
			D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
		));
	}
#endif // DEBUG_MODE
}

// SOFTWARE RASTERIZER
void D3D12App::InitializeSoftwareRasterizer()
{
	BeginFrame();
	//Initialize resources per frame buffer
	SR.InitializeResources(Device);

	// Initialize Shaders
	SR.InitalizeShader(ShaderCompiler);

	// Initialize PSO
	SR.InitializePSO(Device);

	// Initialize data
	SR.InitializeData(Device, Camera, CommandList);

	// MUST BE LAST ONE HERE!!! a.k.a submit everything before executing the compute shader
	{
		EndFrame();
		SR.EndCompute();
	}
}

void D3D12App::RenderSoftwareRasterizer()
{
	//SR.BeginCompute();
	//SR.CopyRenderTarget(Device, RenderTargets[CurrentFrameIdx], CommandList);
	//SR.Compute();
	//SR.EndCompute();
}

void D3D12App::SoftwareRasterizer::InitializeResources(ComPtr<DXDevice>& Device)
{
	// Initialization of command queue
	{
		D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
		CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		CommandQueueDesc.NodeMask = 0; //single GPU env for now
		CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		CommandQueueDesc.Type = COMMAND_LIST_TYPE_COMPUTE;
		CommandQueue = Helpers::CreateCommandQueue(Device.Get(), CommandQueueDesc);
	}

	//Create the command allocator
	{
		ThrowIfFailed(Device->CreateCommandAllocator(COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&CommandAllocator)));
	}

	// Create an event handle to use for frame synchronization + Fence.
	{
		FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (FenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		//Create fence
		ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
	}

	//Initialization of command list
	{
		CommandList = Helpers::CreateGraphicsCommandList(
			Device.Get(),
			CommandListDesc{ 0, COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_FLAG_NONE },
			CommandAllocator.Get(),
			nullptr);

		ThrowIfFailed(CommandList->Close()); //close not closed command list
		// Reset previously used command list and command allocator
		ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));
	}

	// CBV Upload heap
	{
		// create resource for CBV for software rasterizer
		const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ConstantBufferSoftwareRasterizerSize);
		ThrowIfFailed(Device->CreateCommittedResource(
			&DX_HEAP_PROPERTY_UPLOAD,								// this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE,									// no flags
			&UploadBufferDesc,										// size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ,						// will be data that is read from so we keep it in the generic read state
			nullptr,												// we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&ConstantBufferUpload)));
		NAME_D3D12_OBJECT(ConstantBufferUpload, L"Constant Buffer Upload Resource Heap SR");

		// Map
		CD3DX12_RANGE readRange(0, 0);    
		// We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
		// map the resource heap to get a gpu virtual address to the beginning of the heap
		ThrowIfFailed(ConstantBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&CbvGPUAddress)));

		// Memcpy initial data to the constant buffer
		ZeroMemory(&CbvSoftwareRasterizer, ConstantBufferSoftwareRasterizerSize);
		memcpy(CbvGPUAddress, &CbvSoftwareRasterizer, ConstantBufferSoftwareRasterizerSize);
	}

	// SRV/UAV Descriptor Heap
	{
		constexpr auto DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors		= SRVResourceCount + UAVResourceCount; // 3 SRV + 1 UAV
		HeapDesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HeapDesc.Type				= DescriptorHeapType;
		ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&DescriptorHeap)));
	}

	// SRV/UAV resource creation
	{
		const SIZE_T TOTAL_BUFFER_SIZE = Width * Height * 4 * sizeof(float); // resx*resy*4 channels*4 bytes per channel
		IncrementSizeCBVSRVUAV = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// SRV
		{
			// Get handle for 0th descriptor in heap
			CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			{
				// Desc
				const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(TOTAL_BUFFER_SIZE);

				// Color, DepthStencil

				// Create SRV resources for input buffers (2 input buffers)
				for (SIZE_T i = 0; i < 2; ++i)
				{
					// Upload SRV resources for input buffers (2 input buffers)
					ThrowIfFailed(Device->CreateCommittedResource(
						&DX_HEAP_PROPERTY_UPLOAD,
						D3D12_HEAP_FLAG_NONE,
						&BufferDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&SRVResourcesUpload[i])));

					// Real, GPU buffers
					ThrowIfFailed(Device->CreateCommittedResource(
						&DX_HEAP_PROPERTY_DEFAULT,
						D3D12_HEAP_FLAG_NONE,
						&BufferDesc,
						D3D12_RESOURCE_STATE_COMMON,
						nullptr,
						IID_PPV_ARGS(&SRVResources[i])));

					// Create SRV for the input texture
					D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
					SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					SrvDesc.Buffer.NumElements = TOTAL_BUFFER_SIZE / sizeof(float); // resx*resy*4 channels
					SrvDesc.Buffer.StructureByteStride = sizeof(float);
					Device->CreateShaderResourceView(SRVResources[i].Get(), &SrvDesc, SRVHandle);
					SRVHandle.Offset(1, IncrementSizeCBVSRVUAV);
				}
			}

			// Vertex/Color buffer
			{
				// Type of vertex and index

				const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(CompVertexBufferSize);
				// Create SRV resources for input buffers (2 input buffers)
				for (SIZE_T i = 2; i < 2 + 2; ++i)
				{
					// Upload SRV resources for input buffers (2 input buffers)
					ThrowIfFailed(Device->CreateCommittedResource(
						&DX_HEAP_PROPERTY_UPLOAD,
						D3D12_HEAP_FLAG_NONE,
						&BufferDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&SRVResourcesUpload[i])));

					// Real, GPU buffers
					ThrowIfFailed(Device->CreateCommittedResource(
						&DX_HEAP_PROPERTY_DEFAULT,
						D3D12_HEAP_FLAG_NONE,
						&BufferDesc,
						D3D12_RESOURCE_STATE_COMMON,
						nullptr,
						IID_PPV_ARGS(&SRVResources[i])));

					// Create SRV for the input texture
					D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
					SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					SrvDesc.Buffer.NumElements = CompVertexBufferSize / sizeof(float); // 4 channels
					SrvDesc.Buffer.StructureByteStride = sizeof(float);
					Device->CreateShaderResourceView(SRVResources[i].Get(), &SrvDesc, SRVHandle);
					SRVHandle.Offset(1, IncrementSizeCBVSRVUAV);
				}




			}

			// Index buffer
			{
				constexpr UINT IndexBufferPosition = 4;

				const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(CompIndexBufferSize);
				// Create SRV resources for index buffer (1 input buffer)
				ThrowIfFailed(Device->CreateCommittedResource(
					&DX_HEAP_PROPERTY_UPLOAD,
					D3D12_HEAP_FLAG_NONE,
					&BufferDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&SRVResourcesUpload[IndexBufferPosition])));

				// Real, GPU buffers
				ThrowIfFailed(Device->CreateCommittedResource(
					&DX_HEAP_PROPERTY_DEFAULT,
					D3D12_HEAP_FLAG_NONE,
					&BufferDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&SRVResources[IndexBufferPosition])));

				// Create SRV for the input texture
				D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
				SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
				SrvDesc.Buffer.NumElements			= CompIndexBufferSize / (sizeof(uint32_t) * 3);
				SrvDesc.Buffer.StructureByteStride	= sizeof(uint32_t) * 3; // uint32_t * 3 because each triangle has 3 indices
				Device->CreateShaderResourceView(SRVResources[IndexBufferPosition].Get(), &SrvDesc, SRVHandle);
				SRVHandle.Offset(1, IncrementSizeCBVSRVUAV);
			}

		}

		// UAV
		{
			// Desc
			const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(TOTAL_BUFFER_SIZE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			// Get handle for 0th descriptor in heap (after 2 SRVs)
			CD3DX12_CPU_DESCRIPTOR_HANDLE UAVHandle(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			UAVHandle.Offset(SRVResourceCount, IncrementSizeCBVSRVUAV); // because of placement in heap (after 2 SRVs) | SRV | SRV | UAV |
			// Create UAV resources for output buffers (1 output buffer)
			for (SIZE_T i = 0; i < UAVResourceCount; ++i)
			{
				ThrowIfFailed(Device->CreateCommittedResource(
					&DX_HEAP_PROPERTY_DEFAULT,
					D3D12_HEAP_FLAG_NONE,
					&BufferDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&UAVResources[i])));

				// Create UAV for the output texture
				D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
				UavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				UavDesc.Buffer.NumElements = TOTAL_BUFFER_SIZE / sizeof(float); // resx*resy*4 channels
				UavDesc.Buffer.StructureByteStride = sizeof(float);
				UavDesc.Buffer.FirstElement = 0;
				UavDesc.Buffer.CounterOffsetInBytes = 0;
				UavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // or RAW if you want raw (byte-address) access
				Device->CreateUnorderedAccessView(UAVResources[i].Get(), nullptr, &UavDesc, UAVHandle);
				UAVHandle.Offset(1, IncrementSizeCBVSRVUAV);
			}
		}
	}

}

void D3D12App::SoftwareRasterizer::InitalizeShader(D3D12ShaderCompiler& ShaderCompiler)
{
	// Path
	constexpr const wchar_t* ComputeShaderRelativeShaderPath = L"shaders//ComputeShaders//main.hlsl";

	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({L"VERTICES_COUNT", L"4"});

	std::vector<LPCWSTR> arguments;
	// String arguments
	//arguments.push_back(L"-enable-16bit-types");
	//arguments.push_back(L"Qstrip_reflect");
	arguments.push_back(L"-Werror");
	arguments.push_back(L"-Wconversion");
	// Defines arguments
	arguments.push_back(DXC_ARG_ALL_RESOURCES_BOUND);
#if DEBUG_MODE
	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);	//-Od
	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);	//-WX
	arguments.push_back(DXC_ARG_DEBUG);					//-Zi
#else
	arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);	//
#endif

	ComputeShader = ShaderCompiler.CompileShader(ComputeShaderRelativeShaderPath, L"CSMain", L"cs_6_0", arguments, &ShaderDefines);
}

void D3D12App::SoftwareRasterizer::InitializePSO(ComPtr<DXDevice>& Device)
{
	//// CREATE ROOT SIGNATURE
	//// create a root descriptor, which explains where to find the data for this root parameter
	//const auto RootCBVDescriptor = CreateRootDescriptor(0, 0);
	//const auto RootSRVDescriptor1 = CreateRootDescriptor(0, 0);
	//const auto RootCBVDescriptor = CreateRootDescriptor(0, 0);

	//// Create Param
	//// CBV
	//RootParamHelper ParamCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
	//	D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL,
	//	RootCBVDescriptor);



	//// SRV
	//RootParamHelper ParamSRV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_SRV,
	//	D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL);

	//// Create root parameters array
	//auto RootParameters = CreateRootParameters({ ParamCBV, ParamSRV });

	//CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	//RootSignatureDesc.Init(RootParameters.size(), // we have 2 root parameters
	//	RootParameters.data(), // a pointer to the beginning of our root parameters array
	//	1, // we have one static sampler
	//	nullptr, // a pointer to our static sampler (array)
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
	//	D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	//// Initialize Root Signature
	//ComPtr<ID3DBlob> signature;
	//ComPtr<ID3DBlob> error;
	//ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	//ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

	// Create root signature with: [ SRV descriptor table (5) ] [ root CBV (b0) ] [ UAV descriptor table (1) ]
	CD3DX12_DESCRIPTOR_RANGE SrvRange; // t0..t4
	SrvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	CD3DX12_DESCRIPTOR_RANGE UavRange; // u0
	UavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	// Root parameters: root CBV, descriptor table for SRVs, descriptor table for UAVs
	CD3DX12_ROOT_PARAMETER RootParams[3];
	RootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);      // CBV at b0 (root CBV)
	RootParams[1].InitAsDescriptorTable(1, &SrvRange, D3D12_SHADER_VISIBILITY_ALL); // SRVs
	RootParams[2].InitAsDescriptorTable(1, &UavRange, D3D12_SHADER_VISIBILITY_ALL); // UAVs

	// Optional: allow input assembler (common), adjust flags to your needs
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(
		_countof(RootParams),
		RootParams,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// Serialize + create
	ComPtr<ID3DBlob> SerializedRootSig;
	ComPtr<ID3DBlob> ErrorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SerializedRootSig, &ErrorBlob);
	if (FAILED(hr))
	{
		if (ErrorBlob) OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
		ThrowIfFailed(hr);
	}
	ThrowIfFailed(Device->CreateRootSignature(0, SerializedRootSig->GetBufferPointer(), SerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));


	// Describe and create the graphics pipeline state object (PSO).
	D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc = {};
	PsoDesc.CS				= CD3DX12_SHADER_BYTECODE(ComputeShader.Get());
	PsoDesc.pRootSignature	= RootSignature.Get();
	PsoDesc.Flags			= D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(Device->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&PipelineState)));
}

void D3D12App::SoftwareRasterizer::InitializeData(ComPtr<DXDevice>& Device, const ::Camera& AppCamera, ComPtr<DXGraphicsCommandList>& CommandList)
{
	// Constant buffer data for software rasterizer
	{
		// View and Proj mat per frame (camera can move every frame, so we need to update these every frame)
		const auto ViewMat		= AppCamera.GetViewMatrix();			//XMMATRIX
		const auto ProjMat		= AppCamera.GetProjMatrix();			//XMMATRIX
		const auto ViewProjMat	= ViewMat * ProjMat;					//XMMATRIX

		// Constant buffer data for CS
		ZeroMemory(&CbvSoftwareRasterizer, ConstantBufferSoftwareRasterizerSize); // just in case
		XMStoreFloat4x4(&CbvSoftwareRasterizer.ViewMatrix,		XMMatrixTranspose(ViewMat));
		XMStoreFloat4x4(&CbvSoftwareRasterizer.ProjMatrix,		XMMatrixTranspose(ProjMat));
		XMStoreFloat4x4(&CbvSoftwareRasterizer.ViewProjMatrix,	XMMatrixTranspose(ViewProjMat));
		XMStoreFloat4(&CbvSoftwareRasterizer.ScreenSize,		XMVectorSet(static_cast<float>(Width), static_cast<float>(Height), 0.0f, 0.0f));

		// Copy data to mapped region
		memcpy(CbvGPUAddress, &CbvSoftwareRasterizer, ConstantBufferSoftwareRasterizerSize);
	}

	// SRV
	{
		std::vector<float> InputData(Width * Height * 4, 0.0f); // Initialize with zeros
		UINT* GPUVirtualAddress[SRVResourceCount] = {};
		for (SIZE_T i = 0; i < SRVResourceCount - 3; ++i)
		{
			// Map
			CD3DX12_RANGE readRange(0, 0);
			// We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
			// map the resource heap to get a gpu virtual address to the beginning of the heap
			ThrowIfFailed(SRVResourcesUpload[i]->Map(0, &readRange, reinterpret_cast<void**>(&GPUVirtualAddress[i])));

			// Memcpy initial data to the constant buffer
			memcpy(GPUVirtualAddress[i], InputData.data(), Width * Height * 4 * sizeof(float));

			// Actual copy
			{
				// Transition resources to copy destination and copy source states
				const auto Barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[i].Get(),			D3D12_RESOURCE_STATE_COMMON,		D3D12_RESOURCE_STATE_COPY_DEST);
				CommandList->ResourceBarrier(1, &Barrier1);

				// Copy from upload heap to default heap
				CommandList->CopyResource(SRVResources[i].Get(), SRVResourcesUpload[i].Get());

				// Transition resources back to original states
				const auto Barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[i].Get(),			D3D12_RESOURCE_STATE_COPY_DEST,		D3D12_RESOURCE_STATE_COMMON);
				CommandList->ResourceBarrier(1, &Barrier2);
			}
		}
	}
}

void D3D12App::SoftwareRasterizer::BeginCompute()
{
	// Reset previously used command list and command allocator
	{
		ThrowIfFailed(CommandAllocator->Reset());
		ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), PipelineState.Get()));
	}
}

void D3D12App::SoftwareRasterizer::Compute()
{
	// Begin compute (always first!)
	BeginCompute();

	// Actual compute work
	{
		// Descriptor heap for compute shader
		{
			// set the descriptor heap
			DXDescriptorHeap* DescriptorHeaps[] = { DescriptorHeap.Get() };
			CommandList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps);
		}

		// Transition SRVs and UAVs to the states required by the compute shader
		{
			// Transition SRV resources to NON_PIXEL_SHADER_RESOURCE
			std::array<CD3DX12_RESOURCE_BARRIER, SRVResourceCount> SRVBarriers;
			std::array<CD3DX12_RESOURCE_BARRIER, UAVResourceCount> UAVBarriers;
			for (SIZE_T i = 0; i < SRVResourceCount; ++i)
			{
				SRVBarriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
					SRVResources[i].Get(),
					D3D12_RESOURCE_STATE_COMMON, // current state after your uploads
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}
			CommandList->ResourceBarrier(SRVResourceCount, SRVBarriers.data());

			// Transition UAV resources to UNORDERED_ACCESS
			for (SIZE_T i = 0; i < UAVResourceCount; ++i)
			{
				UAVBarriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
					UAVResources[i].Get(),
					D3D12_RESOURCE_STATE_COMMON, // current state at creation
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
			CommandList->ResourceBarrier(UAVResourceCount, UAVBarriers.data());
		}

		// Plane
		PIXBeginEvent(CommandList.Get(), 0, L"Software Rasterizer");
		{
			CommandList->SetPipelineState(PipelineState.Get());
			CommandList->SetComputeRootSignature(RootSignature.Get());

			// CBV at b0 (root CBV)
			CommandList->SetComputeRootConstantBufferView(0, ConstantBufferUpload->GetGPUVirtualAddress());

			// SRV descriptor table (t0..t2)
			CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			CommandList->SetComputeRootDescriptorTable(1, Handle);

			// UAV descriptor table (u0)
			Handle.Offset(SRVResourceCount, IncrementSizeCBVSRVUAV); // because of placement in heap (after 2 SRVs) | SRV | SRV | UAV |
			CommandList->SetComputeRootDescriptorTable(2, Handle);

			// Dispatch compute shader
			//const auto DIV_UP				= [] (UINT a, UINT b) { return (a + b - 1) / b; };
			//const UINT ThreadGroupCountX	= DIV_UP(Width, 8); //
			//const UINT ThreadGroupCountY	= DIV_UP(Height, 4); //
			const auto UAVBarrier			= CD3DX12_RESOURCE_BARRIER::UAV(UAVResources[0].Get());
			CommandList->ResourceBarrier(1, &UAVBarrier); // Ensure UAV writes are visible before dispatch

			//Compute thread-group counts matching [numthreads(8,8,1)]
			const UINT TGX = 8u;
			const UINT TGY = 8u;
			const UINT DispatchX = (Width + TGX - 1) / TGX;
			const UINT DispatchY = (Height + TGY - 1) / TGY;
			CommandList->Dispatch(DispatchX, DispatchY, 1);
			//CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		}
		PIXEndEvent(CommandList.Get());

		// Transition SRVs and UAVs to the states required by the compute shader
		{
			// Transition SRV resources to NON_PIXEL_SHADER_RESOURCE
			std::array<CD3DX12_RESOURCE_BARRIER, SRVResourceCount> SRVBarriers;
			std::array<CD3DX12_RESOURCE_BARRIER, UAVResourceCount> UAVBarriers;
			for (SIZE_T i = 0; i < SRVResourceCount; ++i)
			{
				SRVBarriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
					SRVResources[i].Get(),
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, // current state after your uploads
					D3D12_RESOURCE_STATE_COMMON);
			}
			CommandList->ResourceBarrier(SRVResourceCount, SRVBarriers.data());

			// Transition UAV resources to UNORDERED_ACCESS
			for (SIZE_T i = 0; i < UAVResourceCount; ++i)
			{
				UAVBarriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
					UAVResources[i].Get(),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // current state at creation
					D3D12_RESOURCE_STATE_COMMON);
			}
			CommandList->ResourceBarrier(UAVResourceCount, UAVBarriers.data());
		}
	}

	// End compute (always last!)
	EndCompute();
}

void D3D12App::SoftwareRasterizer::EndCompute()
{
	// Submit necessary things from command list
	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close command list for execution
	DXCommandList* CommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	// Move Fence / Wait for previous compute to end
	SubmitCompute();
}

void D3D12App::SoftwareRasterizer::SubmitCompute()
{
	// Signal and increment the fence value.
	FenceValue = FenceValue + 1;
	// Fence will keep previous value until all commands are completed, thats why FenceValue++;
	ThrowIfFailed(CommandQueue->Signal(Fence.Get(), FenceValue));

	// Wait until recorded commands are processed by GPU.
	if (Fence->GetCompletedValue() < FenceValue)
	{
		ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue, FenceEvent));
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

void D3D12App::SoftwareRasterizer::CopyFromRenderTarget(ComPtr<DXDevice>& Device, const ComPtr<DXResource>& RT, ComPtr<DXGraphicsCommandList>& CommandList)
{	
	// SRV
	{
		// Transition Render Target to Copy Source
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
			RT.Get(),
			RTAfter,
			D3D12_RESOURCE_STATE_COPY_SOURCE
		);
		CommandList->ResourceBarrier(1, &transition);

		// Transition resources to copy destination and copy source states
		const auto Barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[0].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Barrier1);

		// Setup Copies and Copy Texture
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		UINT numRows;
		UINT64 totalBytes;
		const auto RTDesc = RT->GetDesc();
		Device->GetCopyableFootprints(&RTDesc, 0, 1, 0, &layout, &numRows, &totalBytes, nullptr);

		// Create source and destination locations for the copy operation
		CD3DX12_TEXTURE_COPY_LOCATION src(RT.Get());
		CD3DX12_TEXTURE_COPY_LOCATION dest(SRVResources[0].Get(), layout);

		// Copy the UAV resource to the render target
		CommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

		// Transition resources back to original states
		const auto Barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[0].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier2);

		// Transition Render Target back to Render Target
		auto transition2 = CD3DX12_RESOURCE_BARRIER::Transition(
			RT.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			RTAfter
		);
		CommandList->ResourceBarrier(1, &transition2);
	}

}

void D3D12App::SoftwareRasterizer::CopyToRenderTarget(ComPtr<DXDevice>& Device, ComPtr<DXResource>& RT, ComPtr<DXGraphicsCommandList>& CommandList)
{
	// UAV from CS
	{
		// Transition Render Target to Copy Source
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
			RT.Get(),
			RTAfter,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		CommandList->ResourceBarrier(1, &transition);

		// Transition resources to copy destination and copy source states
		const auto Barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(UAVResources[0].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CommandList->ResourceBarrier(1, &Barrier1);

		// Setup Copies and Copy Texture
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		UINT numRows;
		UINT64 totalBytes;
		const auto RTDesc = RT->GetDesc();
		Device->GetCopyableFootprints(&RTDesc, 0, 1, 0, &layout, &numRows, &totalBytes, nullptr);

		// Create source and destination locations for the copy operation
		CD3DX12_TEXTURE_COPY_LOCATION src(UAVResources[0].Get(), layout);
		CD3DX12_TEXTURE_COPY_LOCATION dest(RT.Get());

		// Copy the UAV resource to the render target
		CommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

		// Transition resources back to original states
		const auto Barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(UAVResources[0].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier2);

		// Transition Render Target back to Render Target
		auto transition2 = CD3DX12_RESOURCE_BARRIER::Transition(
			RT.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			RTAfter
		);
		CommandList->ResourceBarrier(1, &transition2);
	}
}

void D3D12App::SoftwareRasterizer::CopyVertexBufferPos(ComPtr<DXDevice>& Device, Helpers::VERTEX_HELPER& VertexUploadToGPU, ComPtr<DXGraphicsCommandList>& CommandList)
{
	// SRV
	{
		constexpr UINT VertexBufferPosition = 2;

		// Transition resources to copy destination and copy source states
		const auto Barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[VertexBufferPosition].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Barrier1);

		// Prepare subresource data pointing to the contiguous array memory
		D3D12_SUBRESOURCE_DATA VertexSubData = {};
		VertexSubData.pData			= reinterpret_cast<const void*>(VertexListComp);
		VertexSubData.RowPitch		= CompVertexBufferSize;
		VertexSubData.SlicePitch	= VertexSubData.RowPitch;

		// Upload to GPU
		UpdateSubresources(CommandList.Get(), SRVResources[VertexBufferPosition].Get(), VertexUploadToGPU.GetPointer(), 0, 0, 1, &VertexSubData);

		// Transition resources back to original states
		const auto Barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[VertexBufferPosition].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier2);
	}
}

void D3D12App::SoftwareRasterizer::CopyVertexBufferColor(ComPtr<DXDevice>& Device, Helpers::VERTEX_HELPER& VertexUploadToGPU2, ComPtr<DXGraphicsCommandList>& CommandList)
{
	// Vertices/Index buffers etc.
	static const VertexPosition VertexListColor[] =
	{
		// Color(4xFloat)
		//  R,		 G,		 B,		 A
		{ 0.5f,		0.5f,	0.5f,	1.0f},	// top left
		{ 0.5f,		0.0f,	0.0f,	1.0f},	// bottom right
		{ 0.5f,		0.5f,	0.0f,	1.0f},	// bottom left
		{ 0.5f,		0.5f,	0.5f,	1.0f},	// top right

		{ 0.0f,		0.5f,	0.5f,	1.0f},	// top left
		{ 0.0f,		0.0f,	0.0f,	1.0f},	// bottom right
		{ 0.0f,		0.5f,	0.0f,	1.0f},	// bottom left
		{ 0.0f,		0.5f,	0.5f,	1.0f},	// top right


		{ 0.5f,		0.0f,	0.5f,	1.0f},	// top left
		{ 0.5f,		0.0f,	0.0f,	1.0f},	// bottom right
		{ 0.5f,		0.0f,	0.0f,	1.0f},	// bottom left
		{ 0.5f,		0.0f,	0.5f,	1.0f},	// top right

		{ 0.0f,		0.5f,	0.0f,	1.0f},	// top left
		{ 0.0f,		0.5f,	0.0f,	1.0f},	// bottom right
		{ 0.0f,		0.5f,	0.0f,	1.0f},	// bottom left
		{ 0.0f,		0.5f,	0.0f,	1.0f}	// top right
	};

	// Type of vertex and index
	constexpr UINT VertexColorBufferSize = sizeof(VertexListColor);
	constexpr UINT VertexBufferColor = 3;

	// Transition resources to copy destination and copy source states
	const auto Barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[VertexBufferColor].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->ResourceBarrier(1, &Barrier1);

	// Prepare subresource data pointing to the contiguous array memory
	D3D12_SUBRESOURCE_DATA VertexSubData = {};
	VertexSubData.pData				= reinterpret_cast<const void*>(VertexListColor);
	VertexSubData.RowPitch			= VertexColorBufferSize;
	VertexSubData.SlicePitch		= VertexSubData.RowPitch;

	// Upload to GPU
	UpdateSubresources(CommandList.Get(), SRVResources[VertexBufferColor].Get(), VertexUploadToGPU2.GetPointer(), 0, 0, 1, &VertexSubData);

	// Transition resources back to original states
	const auto Barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[VertexBufferColor].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	CommandList->ResourceBarrier(1, &Barrier2);
}

void D3D12App::SoftwareRasterizer::CopyIndexBuffer(ComPtr<DXDevice>& Device, Helpers::INDEX_HELPER & IndexUploadToGPU, ComPtr<DXGraphicsCommandList>& CommandList)
{
	// SRV
	{
		constexpr UINT IndexBufferPosition	= 4;

		// Transition resources to copy destination and copy source states
		const auto Barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[IndexBufferPosition].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Barrier1);

		// Prepare subresource data pointing to the contiguous array memory
		D3D12_SUBRESOURCE_DATA IndexSubData = {};
		IndexSubData.pData			= reinterpret_cast<const void*>(IndicesListComp);
		IndexSubData.RowPitch		= CompIndexBufferSize;
		IndexSubData.SlicePitch		= IndexSubData.RowPitch;

		// Upload to GPU
		UpdateSubresources(CommandList.Get(), SRVResources[IndexBufferPosition].Get(), IndexUploadToGPU.GetPointer(), 0, 0, 1, &IndexSubData);

		// Transition resources back to original states
		const auto Barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[IndexBufferPosition].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier2);
	}
}

// SOFTWARE RASTERIZER

void D3D12App::OnKeyDown(UINT8 key)
{
	Camera.OnKeyDown(key);
}

void D3D12App::OnKeyUp(UINT8 key)
{
	Camera.OnKeyUp(key);
}

void D3D12App::InitializePerFrameResources()
{
	DXASSERT(Device && SwapChain, "Device can not be null for Initialization per frame resources");

	// Get handle for 0th descriptor in heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());

	// Resize arrays/vectors per frame buffer resources
	// Fences
	Fences.resize(BACK_BUFFER_COUNT);
	// Fence Values
	FenceValue.resize(BACK_BUFFER_COUNT);
	// Command allocators
	CommandAllocators.resize(BACK_BUFFER_COUNT);
	// Render Targets
	RenderTargets.resize(BACK_BUFFER_COUNT);
	//Constant Buffers
	ConstantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	CbvGPUAddress.resize(BACK_BUFFER_COUNT);

	// Loop
	for (UINT i = 0; i < BACK_BUFFER_COUNT; ++i)
	{
		//Create fences
		ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fences[i])));

		//Create the command allocators
		ThrowIfFailed(Device->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&CommandAllocators[i])));

		// Render target creation
		{
			// Get pointer to swapChain buffer
			ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTargets[i])));

			// Create RenderTargetView 
			Device->CreateRenderTargetView(RenderTargets[i].Get(), nullptr, RtvHandle);

			// Offsets the rtv handle by descriptor size -> new_rtvHandle = rtvHandle + 1 * rtvDescriptorSize
			RtvHandle.Offset(1, RtvIncrementDescriptorSize);
		}

		// CBV Upload heap
		{		
			// create resource for cubes
			const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ConstantBufferPerObjectSize * 3); // HACK! THREE OBJECTS == 3x constant buffer which is per object!
			ThrowIfFailed(Device->CreateCommittedResource(
				&DX_HEAP_PROPERTY_UPLOAD,								// this heap will be used to upload the constant buffer data
				D3D12_HEAP_FLAG_NONE,									// no flags
				&UploadBufferDesc,										// size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
				D3D12_RESOURCE_STATE_GENERIC_READ,						// will be data that is read from so we keep it in the generic read state
				nullptr,												// we do not have use an optimized clear value for constant buffers
				IID_PPV_ARGS(&ConstantBufferUploadHeaps[i])));
			NAME_D3D12_OBJECT(ConstantBufferUploadHeaps[i], L"Constant Buffer Upload Resource Heap");
			ZeroMemory(&CbvPerObject, sizeof(CbvPerObject));

			CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
			// map the resource heap to get a gpu virtual address to the beginning of the heap
			ThrowIfFailed(ConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&CbvGPUAddress[i])));

			// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
			// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
			memcpy(CbvGPUAddress[i], &CbvPerObject, sizeof(CbvPerObject));											// objects's constant buffer data
		}
	}
}

void D3D12App::InitalizeShaders()
{
	// Shaders defines
	//constexpr DxcDefine ShaderDefines[] =
	//{
	//	L"PIXEL_SHADER_ENABLED", L"1",
	//	NULL, NULL
	//};
	//struct DXShaderDefines
	//{
	//	DxcDefine Defines[2] = 
	//	{ { L"PIXEL_SHADER_ENABLED", L"1" }, { NULL, NULL } };
	//	SIZE_T Count;
	//} ShaderDefines;

	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"USE_TEXCOORD", L"1" });

	constexpr const wchar_t* VertexShaderPath	= L"shaders//vertex_shader.hlsl";
	constexpr const wchar_t* PixelShaderPath	= L"shaders//pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;
	// String arguments
	//arguments.push_back(L"-enable-16bit-types");
	//arguments.push_back(L"Qstrip_reflect");
	arguments.push_back(L"-Werror");
	arguments.push_back(L"-Wconversion");
	// Defines arguments
	arguments.push_back(DXC_ARG_ALL_RESOURCES_BOUND);
#if DEBUG_MODE
	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);	//-Od
	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);	//-WX
	arguments.push_back(DXC_ARG_DEBUG);					//-Zi
#else
	arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);	//
#endif

	// We can define shader defines with -D
	//for (const auto& define : ShaderDefines)
	//{
	//	arguments.push_back(L"-D");
	//	arguments.push_back(define.Name);
	//	arguments.push_back(L"=");
	//	arguments.push_back(define.Value);
	//}

	VertexShader	= ShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PixelShader		= ShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

void D3D12App::InitializePSO()
{
	// Pipeline state object (PSO)
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};


	{
		// CREATE ROOT SIGNATURE
		// create a root descriptor, which explains where to find the data for this root parameter
		const auto RootCBVDescriptor = CreateRootDescriptor(0, 0);

		// create a descriptor table
		// create a descriptor range (descriptor table) and fill it out
		// this is a range of descriptors inside a descriptor heap
		const auto DescriptorRange = CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		D3D12_DESCRIPTOR_RANGE DescriptorRangesTable[1] = { DescriptorRange };
		const auto DescriptorTable = CreateRootDescriptorTable(_countof(DescriptorRangesTable), DescriptorRangesTable);

		// Create Param
		// CBV
		RootParamHelper ParamCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
								D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX, 
								RootCBVDescriptor);

		// SRV
		RootParamHelper ParamSRV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
								D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
								DescriptorTable);

		// Create root parameters array
		auto RootParameters = CreateRootParameters({ ParamCBV, ParamSRV });

		// create a static sampler
		D3D12_STATIC_SAMPLER_DESC sampler	= {};
		sampler.Filter						= D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU					= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV					= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW					= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias					= 0;
		sampler.MaxAnisotropy				= 0;
		sampler.ComparisonFunc				= D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor					= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD						= 0.0f;
		sampler.MaxLOD						= D3D12_FLOAT32_MAX;
		sampler.ShaderRegister				= 0;
		sampler.RegisterSpace				= 0;
		sampler.ShaderVisibility			= D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(	RootParameters.size(), // we have 2 root parameters
								RootParameters.data(), // a pointer to the beginning of our root parameters array
								1, // we have one static sampler
								&sampler, // a pointer to our static sampler (array)
								D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
								D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
								D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
								D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		// Initialize Root Signature
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));
	}

	// Depth Stencil Desc
	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc	= {};
	DepthStencilDesc.DepthEnable				= 1;
	DepthStencilDesc.DepthWriteMask				= D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc					= D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	DepthStencilDesc.StencilEnable				= 0;
	DepthStencilDesc.StencilWriteMask			= 0;
	DepthStencilDesc.StencilReadMask			= 0;
	DepthStencilDesc.FrontFace					= {};
	DepthStencilDesc.BackFace					= {};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};
	PsoDesc.InputLayout				= { InputElementDesc, _countof(InputElementDesc) };
	PsoDesc.pRootSignature			= RootSignature.Get();
	PsoDesc.VS						= CD3DX12_SHADER_BYTECODE(VertexShader.Get());
	PsoDesc.PS						= CD3DX12_SHADER_BYTECODE(PixelShader.Get());
	PsoDesc.RasterizerState			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PsoDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.DepthStencilState		= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PsoDesc.DSVFormat				= DEPTH_STENCIL_FORMAT;
	PsoDesc.SampleMask				= UINT_MAX;
	PsoDesc.SampleDesc				= SAMPLE_DESC;
	PsoDesc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PsoDesc.NumRenderTargets		= 1;
	PsoDesc.RTVFormats[0]			= BACK_BUFFER_FORMAT;
	PsoDesc.SampleDesc.Count		= 1;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&PipelineState)));
}

void D3D12App::BeginFrame()
{
	// Reset previously used command list and command allocator
	ThrowIfFailed(CommandAllocators[CurrentFrameIdx]->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocators[CurrentFrameIdx].Get(), PipelineState.Get()));

	// Get Current Frame index 
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Indicate that the back buffer will be used as a render target.
	RTBefore = D3D12_RESOURCE_STATE_PRESENT;
	RTAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	const CD3DX12_RESOURCE_BARRIER BarrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), RTBefore, RTAfter);
	CommandList->ResourceBarrier(1, &BarrierPresentToRTV);

	// Set necessary state.
	CommandList->SetGraphicsRootSignature(RootSignature.Get());
	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHandle(DsvHeap->GetCPUDescriptorHandleForHeapStart());
	CommandList->ClearDepthStencilView(DsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// SV TARGET
	constexpr float ClearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	const CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentFrameIdx, RtvIncrementDescriptorSize);
	CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, &DsvHandle);
	CommandList->ClearRenderTargetView(RtvHandle, ClearColor, 0, nullptr);
}

void D3D12App::EndFrame()
{

	// Upload (intermediate) resource (upload heap)
	Helpers::VERTEX_HELPER VertexUploadToGPU(
		Device.Get(),
		CompVertexBufferSize,
		DX_HEAP_PROPERTY_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		L"VertexUploadToGPUSoftwareRasterizer");

	// Upload (intermediate) resource (upload heap)
	Helpers::VERTEX_HELPER VertexUploadToGPU2(
		Device.Get(),
		CompVertexBufferSize,
		DX_HEAP_PROPERTY_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		L"VertexUploadToGPUSoftwareRasterizer");

	// Upload (intermediate) resource (upload heap)
	Helpers::INDEX_HELPER IndexUploadToGPU(
		Device.Get(),
		CompIndexBufferSize,
		DX_HEAP_PROPERTY_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		L"IndexUploadToGPUSoftwareRasterizer");


	SR.CopyToRenderTarget(		Device,			RenderTargets[CurrentFrameIdx],		CommandList);
	SR.CopyVertexBufferPos(		Device,			VertexUploadToGPU,					CommandList);
	SR.CopyVertexBufferColor(	Device,			VertexUploadToGPU2,					CommandList);
	SR.CopyIndexBuffer(			Device,			IndexUploadToGPU,					CommandList);

	// Update cam pos
	// View and Proj mat per frame (camera can move every frame, so we need to update these every frame)
	const auto ViewMat = Camera.GetViewMatrix();			//XMMATRIX
	const auto ProjMat = Camera.GetProjMatrix();			//XMMATRIX
	const auto ViewProjMat = ViewMat * ProjMat;				//XMMATRIX

	// Constant buffer data for CS
	XMStoreFloat4x4(&SR.CbvSoftwareRasterizer.ViewMatrix,		XMMatrixTranspose(ViewMat));
	XMStoreFloat4x4(&SR.CbvSoftwareRasterizer.ProjMatrix,		XMMatrixTranspose(ProjMat));
	XMStoreFloat4x4(&SR.CbvSoftwareRasterizer.ViewProjMatrix,	XMMatrixTranspose(ViewProjMat));
	XMStoreFloat4(&SR.CbvSoftwareRasterizer.ScreenSize,			XMVectorSet(static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, 0.0f));

	// Copy data to mapped region
	memcpy(SR.CbvGPUAddress, &SR.CbvSoftwareRasterizer, ConstantBufferSoftwareRasterizerSize);


	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER BarrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &BarrierRTVtoPresent);

	RTBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	RTAfter = D3D12_RESOURCE_STATE_PRESENT;

	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close command list for execution
	DXCommandList* CommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	// Present the frame
	ThrowIfFailed(SwapChain->Present(1, 0));

	// Move Fence / Wait for previous frame to end
	WaitForPreviousFrame();
}

void D3D12App::WaitForPreviousFrame()
{
	// Signal and increment the fence value.
	FenceValue[CurrentFrameIdx] = FenceValue[CurrentFrameIdx] + 1;
	// Fence will keep previous value until all commands are completed, thats why FenceValue++;
	ThrowIfFailed(CommandQueue->Signal(Fences[CurrentFrameIdx].Get(), FenceValue[CurrentFrameIdx]));

	// Wait until recorded commands are processed by GPU.
	if (Fences[CurrentFrameIdx]->GetCompletedValue() < FenceValue[CurrentFrameIdx])
	{
		ThrowIfFailed(Fences[CurrentFrameIdx]->SetEventOnCompletion(FenceValue[CurrentFrameIdx], FenceEvent));
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

void D3D12App::FlushCommandList()
{
	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close command list for execution
	DXCommandList* CommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	WaitForPreviousFrame();
}



// NEW SECTION

void D3D12App::StoreCBVDataForPrimitive(const Primitive& Primitive, const UINT PrimitiveIdx)
{
	// View and Proj mat per frame (camera can move every frame, so we need to update these every frame)
	const auto ViewMat = Camera.GetViewMatrix();
	const auto ProjMat = Camera.GetProjMatrix();

	// Primitive Matrices
	const auto PrimitiveWorldMatrix		= Primitive.GetWorldMatrix();

	// WorldToClip (Remember to transpose for HLSL/GPU)
	XMMATRIX LocalToWorld				= XMLoadFloat4x4(&PrimitiveWorldMatrix);						// LocalToWorld
	XMMATRIX WorldToClip				= XMLoadFloat4x4(&PrimitiveWorldMatrix) * ViewMat * ProjMat;	// WorldToClip

	// CBV data for primitive
	PrimitiveConstantBuffer PrimitiveCBVData = {};
	XMStoreFloat4x4(&PrimitiveCBVData.LocalToWorld,		XMMatrixTranspose(LocalToWorld));
	XMStoreFloat4x4(&PrimitiveCBVData.WorldToClip,		XMMatrixTranspose(WorldToClip));

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(PrimitivesConstantBufferPtr + (PrimitiveIdx * PrimitiveConstantBufferSize), &PrimitiveCBVData, PrimitiveConstantBufferSize /* size of */);
}