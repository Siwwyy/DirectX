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
const UINT								BACK_BUFFER_COUNT			= 3;
const D3D_FEATURE_LEVEL					D3D12_FEATURE_LEVEL			= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
const DXGI_FORMAT						BACK_BUFFER_FORMAT			= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
const DXGI_FORMAT						DEPTH_STENCIL_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format
const DXGI_SAMPLE_DESC					SAMPLE_DESC					= SAMPLER_HELPER::CreateSampler(1, 0);


//Globals

// Vertices/Index buffers etc.
TexVertex CubeVertices[] =
{

	//{ -0.5f,  0.5f, 0.5f , 1.0f, 1.0f, 1.0f, 1.0f}, // top left
	//{ 0.5f , -0.5f, 0.5f , 1.0f, 1.0f, 0.0f, 1.0f},	// bottom right
	//{ -0.5f, -0.5f, 0.5f , 1.0f, 0.0f, 0.0f, 1.0f}, // bottom left
	//{  0.5f,  0.5f, 0.5f , 0.0f, 0.0f, 1.0f, 1.0f}, // top right

	{ -0.5f,  0.5f, 0.5f , 0.0f, 0.0f }, // top left
	{ 0.5f , -0.5f, 0.5f , 1.0f, 1.0f }, // bottom right
	{ -0.5f, -0.5f, 0.5f , 0.0f, 1.0f }, // bottom left
	{  0.5f,  0.5f, 0.5f , 1.0f, 0.0f }  // top right
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

	// Objects properties, positions etc.
	Cube.Transform({ 0.0f, 0.1f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 0.5f, 0.5f, 1.f });

	// Main ground plane
	Plane.Transform({ 0.0f, 0.0f, 3.0f }, { -30.0f, 0.0f, 0.0f }, { 5.f, 5.f, 1.f });
}

void D3D12App::Initialize()
{
	HMODULE module = LoadLibrary(L"WinPixEventRuntime.dll");
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

	/***************************
	 ***** Constant Buffers ****
	 ********* Matrices ********
	 ***************************/
	{
		// set starting square position
		// set starting scale, rotation and position
		const XMFLOAT4 ScaleTransform		= { 0.2f, 0.2f, 0.2f, 1.f };
		const XMFLOAT4 RotateTransform		= { 0.f, 0.f, 0.f, 1.f };
		const XMFLOAT4 PositionOffsetVec	= { 0.f, 0.f, -0.3f, 1.f };

		// create xmvector for object world matrix
		const auto ScaleMat			= XMMatrixScaling(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z);
		const auto RotationMatX		= XMMatrixRotationX(RotateTransform.x);
		const auto RotationMatY		= XMMatrixRotationY(RotateTransform.y);
		const auto RotationMatZ		= XMMatrixRotationZ(RotateTransform.z);
		const auto RotationMatXYZ	= RotationMatX * RotationMatY * RotationMatZ;

		// Offset of translation
		const auto PositionOld = SquareMatrices.PositionVec;
		const auto PositionNew = XMLoadFloat4(&PositionOld) + XMLoadFloat4(&PositionOffsetVec);

		// Store
		SquareMatrices.ScaleVec = ScaleTransform;
		SquareMatrices.RotVec	= RotateTransform;
		XMStoreFloat4(&SquareMatrices.PositionVec, PositionNew);

		// create translation matrix
		const auto PositionMat = XMMatrixTranslation(
			SquareMatrices.PositionVec.x, 
			SquareMatrices.PositionVec.y, 
			SquareMatrices.PositionVec.z);

		// Create World Matrix, Scale * Rotation * Position
		const auto WorldMatrix = ScaleMat * RotationMatXYZ * PositionMat;
		XMStoreFloat4x4(&SquareMatrices.WorldMat, WorldMatrix);
	}
	/////////

	// Test
	RenderGraph Graph(Device, CommandList);
	{
		// Init

		// Create resources
		// Vertex Buffer
		auto VertexDescGPU		= BufferDesc::CreateBufferDesc(VertexBufferSize, 1, BufferType::VertexBuffer, DX_HEAP_PROPERTY_DEFAULT, D3D12_RESOURCE_STATE_COMMON,		L"VertexBufferGPU");
		auto VertexDescCPU		= BufferDesc::CreateBufferDesc(VertexBufferSize, 1, BufferType::VertexBuffer, DX_HEAP_PROPERTY_UPLOAD,	D3D12_RESOURCE_STATE_GENERIC_READ,	L"VertexBufferCPU");
		auto VertexBufferGPU	= Graph.CreateBuffer(VertexDescGPU);
		auto VertexBufferCPU	= Graph.CreateBuffer(VertexDescCPU);

		// IndexBuffer
		auto IndexDescGPU		= BufferDesc::CreateBufferDesc(IndexBufferSize, 1, BufferType::IndexBuffer, DX_HEAP_PROPERTY_DEFAULT,	D3D12_RESOURCE_STATE_COMMON,		L"IndexBufferGPU");
		auto IndexDescCPU		= BufferDesc::CreateBufferDesc(IndexBufferSize, 1, BufferType::IndexBuffer, DX_HEAP_PROPERTY_UPLOAD,	D3D12_RESOURCE_STATE_GENERIC_READ,	L"IndexBufferCPU");
		auto IndexBufferGPU		= Graph.CreateBuffer(IndexDescGPU);
		auto IndexBufferCPU		= Graph.CreateBuffer(IndexDescCPU);

		// AddPass
		Graph.AddPass(TEXT("VertexSquare"), {&VertexBufferGPU, &VertexBufferCPU}, [=, &VertexBufferGPU, &VertexBufferCPU](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			/***************************
			****** VERTEX BUFFER ******
			***************************/
			constexpr const auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
			constexpr const auto StateAfter		= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the vertex buffer.
			// store vertex buffer in upload heap
			D3D12_SUBRESOURCE_DATA	VertexData = {};
			VertexData.pData		= reinterpret_cast<UINT8*>(CubeVertices);
			VertexData.RowPitch		= VertexBufferSize;
			VertexData.SlicePitch	= VertexData.RowPitch;

			//PIXBeginEvent(CommandList.Get(), 0, L"Copy vertex buffer data to default resource...");

			// transition 
			RenderGraph::TransitionBarrier(CommandList, &VertexBufferGPU, StateBefore);

			// Update Subresource
			UpdateSubresources(CommandList, VertexBufferGPU.Resource.Get(), VertexBufferCPU.Resource.Get(), 0, 0, 1, &VertexData);

			// transition the vertex buffer data from copy destination state to vertex buffer state
			RenderGraph::TransitionBarrier(CommandList, &VertexBufferGPU, StateAfter);

			// Release the resources
			VertexBuffer		= VertexBufferGPU.Resource;
			VertexBufferView	= RenderGraph::CreateVertexBufferView(VertexBuffer->GetGPUVirtualAddress(), sizeof(TexVertex), VertexBufferSize);
		});

		// AddPass
		Graph.AddPass(TEXT("IndexSquare"), { &IndexBufferGPU, &IndexBufferCPU }, [=, &IndexBufferGPU, &IndexBufferCPU](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			/***************************
			****** INDEX BUFFER ******
			***************************/
			constexpr const auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
			constexpr const auto StateAfter		= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the vertex buffer.
			// store index buffer in upload heap
			D3D12_SUBRESOURCE_DATA	IndexData = {};
			IndexData.pData			= reinterpret_cast<UINT8*>(Indices);	// pointer to our index array
			IndexData.RowPitch		= IndexBufferSize;						 // size of all our index buffer
			IndexData.SlicePitch	= IndexData.RowPitch;					 // also the size of our index buffer

			//PIXBeginEvent(CommandList.Get(), 0, L"Copy vertex buffer data to default resource...");

			// transition 
			RenderGraph::TransitionBarrier(CommandList, &IndexBufferGPU, StateBefore);

			// Update Subresource
			UpdateSubresources(CommandList, IndexBufferGPU.Resource.Get(), IndexBufferCPU.Resource.Get(), 0, 0, 1, &IndexData);

			// transition the vertex buffer data from copy destination state to vertex buffer state
			RenderGraph::TransitionBarrier(CommandList, &IndexBufferGPU, StateAfter);

			// Release the resources
			IndexBuffer		= IndexBufferGPU.Resource;
			IndexBufferView = RenderGraph::CreateIndexBufferView(IndexBuffer->GetGPUVirtualAddress(), DXGI_FORMAT_R32_UINT, IndexBufferSize);
		});

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
		Graph.AddPass(TEXT("Cube Init"), {}, [=](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			Cube.Init(Device,			CommandList);
			Texture.Load(Device,		CommandList,	L"bryanzar.png",	MainDescriptorHeap, IncrementDescriptorSize, 0);
			TextureMegane.Load(Device,	CommandList,	L"megane.jpg",		MainDescriptorHeap, IncrementDescriptorSize, 1);
		});

		/***************************
		 ********** Plane **********
		 ***************************/
		Graph.AddPass(TEXT("Plane Init"), {}, [=](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			Plane.Init(Device, CommandList);
		});
	}


	// Execution
	Graph.Execute();

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
		ID3D12DescriptorHeap* DescriptorHeaps[] = { MainDescriptorHeap.Get()};
		CommandList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps);
	}

	////Square
	//{
	//	// set objects's constant buffer
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//	Handle.Offset(1, IncrementDescriptorSize);
	//	CommandList->SetGraphicsRootDescriptorTable(1, Handle);
	//	CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + (0 * ConstantBufferPerObjectSize));
	//	CommandList->IASetVertexBuffers(0, 1, &VertexBufferView); // set the vertex buffer (using the vertex buffer view)
	//	CommandList->IASetIndexBuffer(&IndexBufferView);
	//	CommandList->DrawIndexedInstanced(NumIndices, 1, 0, 0, 0); // draw 2 triangles (draw 1 instance of 2 triangles)
	//}

	// Cube
	{
		//CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		//Handle.Offset(1, IncrementDescriptorSize);
		//CommandList->SetGraphicsRootDescriptorTable(1, Handle);
		//CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + (1 * ConstantBufferPerObjectSize));
		//CommandList->IASetVertexBuffers(0, 1, &Cube.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		//CommandList->IASetIndexBuffer(&Cube.IndexBufferView);
		//CommandList->DrawIndexedInstanced(Cube.GetNumIndices(), 1, 0, 0, 0); // draw cube
	}

	// Plane
	PIXBeginEvent(CommandList.Get(), 0, L"Post-processing");
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		CommandList->SetGraphicsRootDescriptorTable(1, Handle);
		CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + (2 * ConstantBufferPerObjectSize));
		CommandList->IASetVertexBuffers(0, 1, &Plane.VertexFactory.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		CommandList->IASetIndexBuffer(&Plane.IndexBufferView);
		CommandList->DrawIndexedInstanced(Plane.GetNumIndices(), 1, 0, 0, 0); // draw plane
	}
	PIXEndEvent(CommandList.Get());

	//Always EndFrame last
	EndFrame();
}

void D3D12App::Update(float DeltaTime)
{
	// Other Updates
	step_timer.Tick(NULL);
	if (CurrentFrameIdx % 509)
	{
		// Update window text with FPS value.
		wchar_t fps[64];
		swprintf_s(fps, L"%ufps", step_timer.GetFramesPerSecond());
		//SetCustomWindowText(fps);
		std::wstring WindowText		= L"My Window | FPS: ";
		std::wstring WindowText2	= fps;
		SetWindowText(Win32Proc::GetHwnd(), (WindowText + WindowText2).c_str());
	}

	// Camera matrices
	Camera.Update(static_cast<float>(step_timer.GetElapsedSeconds()));
	const auto ViewMat	= Camera.GetViewMatrix();
	const auto ProjMat	= Camera.GetProjMatrix();

	// update app logic, such as moving the camera or figuring out what objects are in view

	// create xmvector for object world matrix
	const auto ScaleMat			= XMMatrixScaling(SquareMatrices.ScaleVec.x, SquareMatrices.ScaleVec.y, SquareMatrices.ScaleVec.z);
	const auto RotationMatX		= XMMatrixRotationX(SquareMatrices.RotVec.x);
	const auto RotationMatY		= XMMatrixRotationY(SquareMatrices.RotVec.y);
	const auto RotationMatZ		= XMMatrixRotationZ(SquareMatrices.RotVec.z);
	const auto RotationMatXYZ	= RotationMatX * RotationMatY * RotationMatZ;

	// Offset of translation
	const auto PositionOld = SquareMatrices.PositionVec;
	const auto PositionNew = XMLoadFloat4(&PositionOld) + DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f); // no movement yet

	// create translation matrix      
	XMStoreFloat4(&SquareMatrices.PositionVec, PositionNew);                                                                        // initialize object's rotation matrix to identity matrix
	const auto PositionMat = XMMatrixTranslation(
		SquareMatrices.PositionVec.x,
		SquareMatrices.PositionVec.y,
		SquareMatrices.PositionVec.z);

	// Create World Matrix, Scale * Rotation * Position
	const auto WorldMatrix = ScaleMat * RotationMatXYZ * PositionMat;
	XMStoreFloat4x4(&SquareMatrices.WorldMat, WorldMatrix);


	// update constant buffer for object
	// create the wvp matrix and store in constant buffer
	XMMATRIX MVPMat				= XMLoadFloat4x4(&SquareMatrices.WorldMat) * ViewMat * ProjMat; // create wvp matrix
	XMMATRIX Transposed			= XMMatrixTranspose(MVPMat);									// must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerObject.WorldViewProjectionMat4x4, Transposed);						// store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx] + (0 * ConstantBufferPerObjectSize), &CbvPerObject, sizeof(CbvPerObject));


	// Cube

	// store cube1's world matrix
	static float z_offset		= 0.0001f;
	static float x_rot_offset	= 0.001f;
	Cube.Transform(DX_IDENTITY_TRANSFORM3, { x_rot_offset, x_rot_offset, 0.0f });
	// update constant buffer for cube1
	// create the wvp matrix and store in constant buffer
	const auto CubeWorldMatrix	= Cube.GetWorldMatrix();
	XMMATRIX MVPMatCube			= XMLoadFloat4x4(&CubeWorldMatrix) * ViewMat * ProjMat; // create wvp matrix
	XMMATRIX TransposedCube		= XMMatrixTranspose(MVPMatCube); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerObject.WorldViewProjectionMat4x4, TransposedCube); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx] + (1 * ConstantBufferPerObjectSize), &CbvPerObject, sizeof(CbvPerObject));

	// store plane's world matrix
	// create the wvp matrix and store in constant buffer
	const auto PlaneWorldMatrix = Plane.GetWorldMatrix();
	XMMATRIX MVPMatPlane		= XMLoadFloat4x4(&PlaneWorldMatrix) * ViewMat * ProjMat;	// create wvp matrix
	XMMATRIX TransposedPlane	= XMMatrixTranspose(MVPMatPlane);							// must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerObject.WorldViewProjectionMat4x4, TransposedPlane);				// store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx] + (2 * ConstantBufferPerObjectSize), &CbvPerObject, sizeof(CbvPerObject));
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
	// Shaders compilation //TODO
	// Shaders defines
	//constexpr DxcDefine ShaderDefines[] =
	//{
	//	L"PIXEL_SHADER_ENABLED", L"1",
	//	NULL, NULL
	//};
	constexpr DxcDefine ShaderDefines[] =
	{
		L"USE_TEXCOORD", L"1",
		NULL, NULL
	};

	constexpr const wchar_t* VertexShaderPath	= L"shaders//vertex_shader.hlsl";
	constexpr const wchar_t* PixelShaderPath	= L"shaders//pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;
	// String arguments
	//arguments.push_back(L"-enable-16bit-types");
	//arguments.push_back(L"Qstrip_reflect");
	//arguments.push_back(L"Qstrip_debug");
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

	VertexShader	= ShaderCompiler.CompileShader(VertexShaderPath, ShaderDefines, L"main", L"vs_6_0", arguments);
	PixelShader		= ShaderCompiler.CompileShader(PixelShaderPath, ShaderDefines, L"main", L"ps_6_0", arguments);
}

void D3D12App::InitializePSO()
{
	// Pipeline state object (PSO)
	// Define the vertex input layout.
	//D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	//};

	//D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	//};

	D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	PsoDesc.InputLayout				= { InputElementDescs, _countof(InputElementDescs) };
	PsoDesc.pRootSignature			= RootSignature.Get();
	PsoDesc.VS						= CD3DX12_SHADER_BYTECODE(VertexShader.Get());
	PsoDesc.PS						= CD3DX12_SHADER_BYTECODE(PixelShader.Get());
	PsoDesc.RasterizerState			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PsoDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.DepthStencilState		= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//PsoDesc.DepthStencilState		= DepthStencilDesc;
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
	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close command list for execution
	DXCommandList* CommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	WaitForPreviousFrame();
}