#include "D3D12App.h"


// Own Includes
#include "Win32Proc.h"
#include "D3D12Math.h"
#include "Text.h"

#include <string>

// Namespaces
using namespace Helpers;


//CONSTANTS
const D3D12_COMMAND_LIST_TYPE			COMMAND_LIST_TYPE			= D3D12_COMMAND_LIST_TYPE_DIRECT;
const UINT								BACK_BUFFER_COUNT			= 3;
const D3D_FEATURE_LEVEL					D3D12_FEATURE_LEVEL			= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
const DXGI_FORMAT						BACK_BUFFER_FORMAT			= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
const DXGI_FORMAT						DEPTH_STENCIL_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format
const DXGI_SAMPLE_DESC					SAMPLE_DESC					= SAMPLER_HELPER::CreateSampler(1, 0);


//Globals

// Vertices/Index buffers etc.
Vertex CubeVertices[] =
{
	//{ 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },


	//{ 0.0f, 0.25f , 0.0f , 1.0f, 0.0f, 0.0f, 1.0f  },
	//{ 0.25f, -0.25f , 0.0f,  0.0f, 1.0f, 0.0f, 1.0f  },
	//{ -0.25f, -0.25f , 0.0f , 0.0f, 0.0f, 1.0f, 1.0f  },
	//{ -0.25f, -0.25f , 0.0f , 0.0f, 0.0f, 1.0f, 1.0f  }

	{ -0.5f,  0.5f, 0.5f , 1.0f, 1.0f, 1.0f, 1.0f}, // top left
	{ 0.5f , -0.5f, 0.5f , 1.0f, 1.0f, 0.0f, 1.0f},	// bottom right
	{ -0.5f, -0.5f, 0.5f , 1.0f, 0.0f, 0.0f, 1.0f}, // bottom left
	{  0.5f,  0.5f, 0.5f , 0.0f, 0.0f, 1.0f, 1.0f}, // top right



	//// front face
	//{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	//{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	//// right side face
	//{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	//// left side face
	//{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	//{ -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	//// back face
	//{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	//// top face
	//{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	//{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	//// bottom face
	//{  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f }



	/////////////////////////////
	//{ -0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{  0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f }
	/////////////////////////////

	////With UV Tex coord
	//// front face
	//{ -0.5f, 0.5f, -0.5f, 0.0f, 0.0f },
	//{ 0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
	//{ -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
	//{ 0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

	//// right side face
	//{ 0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
	//{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
	//{ 0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
	//{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

	//// left side face
	//{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
	//{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
	//{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
	//{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

	//// back face
	//{ 0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
	//{ -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
	//{ 0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
	//{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

	//// top face
	//{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
	//{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
	//{ 0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
	//{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

	//// bottom face
	//{ 0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
	//{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
	//{ 0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
 
};

constexpr UINT VertexBufferSize = sizeof(CubeVertices);

DWORD Indices[] =
{
	//// front face
	//0, 1, 2, // first triangle
	//0, 3, 1, // second triangle

	//// left face
	//4, 5, 6, // first triangle
	//4, 7, 5, // second triangle

	//// right face
	//8, 9, 10, // first triangle
	//8, 11, 9, // second triangle

	//// back face
	//12, 13, 14, // first triangle
	//12, 15, 13, // second triangle

	//// top face
	//16, 17, 18, // first triangle
	//16, 19, 17, // second triangle

	//// bottom face
	//20, 21, 22, // first triangle
	//20, 23, 21, // second triangle

	///////////////////////////////
	0, 1, 2,
	0, 3, 1
	// Its order is because its clock-wise when using Triangle List topology
	// See: https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-primitive-topologies

	///////////////////////////////
};

constexpr UINT IndexBufferSize	= sizeof(Indices);
constexpr UINT NumIndices		= IndexBufferSize / sizeof(DWORD);

//////////////////////////////////////////////////////////////////



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
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");

	//Objects properties
	//Cube.SetPosVector(XMFLOAT4(-1.8f, -0.8f, 1.f, 0.0f));
	//Cube.Transform(XMFLOAT3(0.1f, 1.f, 1.f), {}, {});
	Camera.SetPosVector(XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0));
}

void D3D12App::Initialize()
{
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
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0; //single GPU env for now
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type = COMMAND_LIST_TYPE;
	CommandQueue = Helpers::CreateCommandQueue(Device.Get(), CommandQueueDesc);

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
	D3D12_CLEAR_VALUE DepthOptimizedClearValue = {};
	DepthOptimizedClearValue.Format = DEPTH_STENCIL_FORMAT;
	DepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	DepthOptimizedClearValue.DepthStencil.Stencil = 0;

	// Create Depth Stencil Texture 2D
	const auto DepthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(DEPTH_STENCIL_FORMAT, WindowWidth, WindowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
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


	/***************************
	 ****** VERTEX BUFFER ******
	 ***************************/
	{
		constexpr auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
		constexpr auto StateAfter	= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		// GPU Vertex
		Helpers::VERTEX_HELPER VertexGPU(Device.Get(),
			VertexBufferSize,
			DX_HEAP_PROPERTY_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON,
			L"VertexGPU");

		// Upload Vertex
		Helpers::VERTEX_HELPER VertexUploadToGPU(Device.Get(),
			VertexBufferSize,
			DX_HEAP_PROPERTY_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			L"VertexUploadToGPU");

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA VertexData = {};
		VertexData.pData		= reinterpret_cast<UINT8*>(CubeVertices);
		VertexData.RowPitch		= VertexBufferSize;
		VertexData.SlicePitch	= VertexData.RowPitch;

		//PIXBeginEvent(CommandList.Get(), 0, L"Copy vertex buffer data to default resource...");

		// Update Subresource
		UpdateSubresources(CommandList.Get(), VertexGPU.GetPointer(), VertexUploadToGPU.GetPointer(), 0, 0, 1, &VertexData);

		// transition the vertex buffer data from copy destination state to vertex buffer state
		const auto VertexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexGPU.GetPointer(), StateBefore, StateAfter);
		CommandList->ResourceBarrier(1, &VertexCmdListBarrier);

		// Release the resources
		VertexBufferView	= VertexGPU.CreateView(sizeof(Vertex), VertexBufferSize);
		VertexBuffer		= VertexGPU.ReleaseResource();

		{
			// Submit necessary things from command list
			// Execute command lists
			ThrowIfFailed(CommandList->Close()); //close command list for execution
			DXCommandList* CommandLists[] = { CommandList.Get() };
			CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

			// Move Fence / Wait for previous frame to end
			WaitForPreviousFrame();

			// Reset previously used command list and command allocator
			ThrowIfFailed(CommandList->Reset(CommandAllocators[0].Get(), PipelineState.Get()));
		}

	}

	///////

	/***************************
	 ****** INDEX BUFFER *****
	 ***************************/
	{
		// GPU Vertex
		constexpr auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
		constexpr auto StateAfter	= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		Helpers::INDEX_HELPER IndexGPU(Device.Get(),
			IndexBufferSize,
			DX_HEAP_PROPERTY_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON,
			L"IndexGPU");

		// Upload Vertex
		Helpers::INDEX_HELPER IndexUploadToGPU(Device.Get(),
			IndexBufferSize,
			DX_HEAP_PROPERTY_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			L"IndexUploadToGPU");

		// store index buffer in upload heap
		D3D12_SUBRESOURCE_DATA IndexData = {};
		IndexData.pData			= reinterpret_cast<UINT8*>(Indices); // pointer to our index array
		IndexData.RowPitch		= IndexBufferSize;						 // size of all our index buffer
		IndexData.SlicePitch	= IndexData.RowPitch;					 // also the size of our index buffer

		// Update Subresource
		UpdateSubresources<1>(CommandList.Get(), IndexGPU.GetPointer(), IndexUploadToGPU.GetPointer(), 0, 0, 1, &IndexData);

		// transition the vertex buffer data from copy destination state to vertex buffer state
		const auto IndexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(IndexGPU.GetPointer(), StateBefore, StateAfter);
		CommandList->ResourceBarrier(1, &IndexCmdListBarrier);

		// Release the resources
		IndexBufferView = IndexGPU.CreateView(IndexBufferSize, DXGI_FORMAT_R32_UINT);
		IndexBuffer		= IndexGPU.ReleaseResource();

		{
			// Submit necessary things from command list
			// Execute command lists
			ThrowIfFailed(CommandList->Close()); //close command list for execution
			DXCommandList* CommandLists[] = { CommandList.Get() };
			CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

			// Move Fence / Wait for previous frame to end
			WaitForPreviousFrame();

			// Reset previously used command list and command allocator
			ThrowIfFailed(CommandList->Reset(CommandAllocators[0].Get(), PipelineState.Get()));
		}

	}

	/////////

	/***************************
	 ***** Constant Buffers ****
	 ********* Matrices ********
	 ***************************/
	{
		// set starting cubes position
		// first cube
		SquareMatrices.Position = XMFLOAT4(0.0f, 0.0f, 4.0f, 0.0f);		// set object position
		XMVECTOR PosVec = XMLoadFloat4(&SquareMatrices.Position);		// create xmvector for object position
		auto TmpMat		= XMMatrixTranslationFromVector(PosVec);		// create translation matrix from object's position vector
		XMStoreFloat4x4(&SquareMatrices.RotMat, XMMatrixIdentity());	// initialize object's rotation matrix to identity matrix
		XMStoreFloat4x4(&SquareMatrices.WorldMat, TmpMat);				// store object's world matrix
	}
	/////////

	/***************************
	 *********** Cube **********
	 ***************************/
	{
		Cube.Init(Device.Get(), CommandList.Get());

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
}

void D3D12App::Render()
{
	//Always BeginFrame first
	BeginFrame();

	// Drawing


	//Square
	{
		// set objects's constant buffer
		CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress());
		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		CommandList->IASetIndexBuffer(&IndexBufferView);
		CommandList->DrawIndexedInstanced(NumIndices, 1, 0, 0, 0); // draw 2 triangles (draw 1 instance of 2 triangles)
	}

	// Cube
	{
		CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + ConstantBufferPerObjectSize);
		CommandList->IASetVertexBuffers(0, 1, &Cube.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		CommandList->IASetIndexBuffer(&Cube.IndexBufferView);
		CommandList->DrawIndexedInstanced(Cube.GetNumIndices(), 1, 0, 0, 0); // draw cube
	}

#if 0
	RenderText(CommandList.Get(), 
		DsvHeap.Get(), 
		CurrentFrameIdx, 
		arialFont, 
		std::wstring(L"FPS: ") + std::to_wstring(timer.fps), 
		XMFLOAT2(0.02f, 0.01f),
		XMFLOAT2(2.0f, 2.0f),
		XMFLOAT2(0.0f, 0.0f),
		XMFLOAT4(1.0f, 1.0f, 1.f, 1.f));
#endif
	//Always EndFrame last
	EndFrame();
}

void D3D12App::Update(float DeltaTime)
{
	// Camera matrices
	const auto CameraViewMat	= Camera.GetViewMatrix();
	const auto CameraProjMat	= Camera.GetProjMatrix();
	XMMATRIX ViewMat			= XMLoadFloat4x4(&CameraViewMat); // load view matrix
	XMMATRIX ProjMat			= XMLoadFloat4x4(&CameraProjMat); // load projection matrix

	// update app logic, such as moving the camera or figuring out what objects are in view

	// create rotation matrices
	XMMATRIX RotXMat = XMMatrixRotationX(0.0001f);
	XMMATRIX RotYMat = XMMatrixRotationY(0.002f);
	XMMATRIX RotZMat = XMMatrixRotationZ(0.003f);

	// add rotation to object's rotation matrix and store it
	XMMATRIX RotMat = XMLoadFloat4x4(&SquareMatrices.RotMat);// *rotYMat * rotZMat; // * rotXMat * rotYMat * rotZMat;
	XMStoreFloat4x4(&SquareMatrices.RotMat, RotMat);

	// create translation matrix for object position vector
	XMMATRIX TranslationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&SquareMatrices.Position));

	// create object's world matrix by first rotating the cube, then positioning the rotated cube
	XMMATRIX WorldMat = RotMat * TranslationMat;

	// store object's world matrix
	XMStoreFloat4x4(&SquareMatrices.WorldMat, WorldMat);

	// update constant buffer for object
	// create the wvp matrix and store in constant buffer
	XMMATRIX MVPMat				= XMLoadFloat4x4(&SquareMatrices.WorldMat) * ViewMat * ProjMat; // create wvp matrix
	XMMATRIX Transposed			= XMMatrixTranspose(MVPMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerObject.WorldViewProjectionMat4x4, Transposed); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx], &CbvPerObject, sizeof(CbvPerObject));


	// Cube

	//// store cube1's world matrix
	static float z_offset = 0.001f;
	//Cube.Transform(XMFLOAT3(1.f, 1.f, 1.f), XMFLOAT3(0.f, 0.f, 0.f), { 0.f, 0.f, z_offset });
	Cube.Transform({ 0.f, 0.f, z_offset });
	//z_offset += 0.0001f;
	// update constant buffer for cube1
	// create the wvp matrix and store in constant buffer
	const auto CubeWorldMatrix	= Cube.GetWorldMatrix();
	XMMATRIX MVPMatCube			= XMLoadFloat4x4(&CubeWorldMatrix) * ViewMat * ProjMat; // create wvp matrix
	XMMATRIX TransposedCube		= XMMatrixTranspose(MVPMatCube); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerObject.WorldViewProjectionMat4x4, TransposedCube); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx] + ConstantBufferPerObjectSize, &CbvPerObject, sizeof(CbvPerObject));
}

void D3D12App::Destroy()
{
	CloseHandle(FenceEvent);

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
			const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(CalculateConstantBufferByteSize(256), 
				D3D12_RESOURCE_FLAG_NONE,
				D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
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
	// Shaders compilation //TODO
	// Shaders defines
	constexpr DxcDefine ShaderDefines[] =
	{
		L"PIXEL_SHADER_ENABLED", L"1",
		NULL, NULL
	};

	constexpr const wchar_t* VertexShaderPath	= L"shaders//vertex_shader.hlsl";
	constexpr const wchar_t* PixelShaderPath	= L"shaders//pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;
#if DEBUG_MODE
	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS); //-Od
	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
	arguments.push_back(DXC_ARG_DEBUG); //-Zi
#endif

	// We can define shader defines with -D
	//for (const auto& define : ShaderDefines)
	//{
	//	arguments.push_back(L"-D");
	//	arguments.push_back(define.Name);
	//	arguments.push_back(L"=");
	//	arguments.push_back(define.Value);
	//}

	VertexShader	= ShaderCompiler.CompileShader(VertexShaderPath, nullptr, L"main", L"vs_6_0", arguments);
	PixelShader		= ShaderCompiler.CompileShader(PixelShaderPath, ShaderDefines, L"main", L"ps_6_0", arguments);
}

void D3D12App::InitializePSO()
{
	// Pipeline state object (PSO)
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	//// fill out an input layout description structure
	//D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};
	//// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	//InputLayoutDesc.NumElements = sizeof(InputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	//InputLayoutDesc.pInputElementDescs = InputElementDescs;

	//// Create an empty root signature.
	//{
	//	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	//	RootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//	ComPtr<ID3DBlob> signature;
	//	ComPtr<ID3DBlob> error;
	//	ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	//	ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));
	//}


	{
		// create root signature

		// create a root descriptor, which explains where to find the data for this root parameter
		D3D12_ROOT_DESCRIPTOR RootCBVDescriptor;
		RootCBVDescriptor.RegisterSpace		= 0;
		RootCBVDescriptor.ShaderRegister	= 0;

		// create a root parameter and fill it out
		D3D12_ROOT_PARAMETER  RootParameters[1]; // only one parameter right now
		RootParameters[0].ParameterType		= D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
		RootParameters[0].Descriptor		= RootCBVDescriptor; // this is the root descriptor for this root parameter
		RootParameters[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(_countof(RootParameters), // we have 1 root parameter
			RootParameters, // a pointer to the beginning of our root parameters array
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));
	}

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};
	PsoDesc.InputLayout				= { InputElementDescs, _countof(InputElementDescs) };
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
	const CD3DX12_RESOURCE_BARRIER BarrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
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
	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER BarrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &BarrierRTVtoPresent);

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
	WaitForPreviousFrame();
}
