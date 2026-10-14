#include "D3D12App.h"


// Own Includes
#include "Win32Proc.h"
#include "D3D12Math.h"
#include "RenderGraph.h"
#include "Paths.h"

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
const UINT								MAX_MATERIALS				= 10;					//how many materials fit in the shared descriptor heap

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
	, PlaneMaterial(TEXT("Plane Material"))
	, PrimitiveMaterial(TEXT("Primitive Material"))
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
		// Every material takes its descriptors out of this single heap
		SharedMaterialHeap.Initialize(Device.Get(), MAX_MATERIALS);
		PlaneMaterial.Initialize(Device.Get(), SharedMaterialHeap);
		PrimitiveMaterial.Initialize(Device.Get(), SharedMaterialHeap);

		// AddPass
		Graph.AddPass(TEXT("Cubes Init"), {}, [=](DXDevice * Device, DXGraphicsCommandList * CommandList)
		{
			//Cube1.Init(Device,			CommandList);
			//Cube2.Init(Device,			CommandList);
			PlaneMaterial.LoadTexture(Device,		CommandList,	MaterialTextureSlot::Diffuse,			(DXIPATHS::GetTextureDir / L"bryanzar.png").Str());

			// Full CC0 map set from Poly Haven, DirectX normal variant so the green channel needs no flip.
			// Only Diffuse is sampled today, the rest sit in the table waiting for the shader to use them.
			const auto MetalPlateDir = DXIPATHS::GetTextureDir / L"metal_plate";
			PrimitiveMaterial.LoadTexture(Device,	CommandList,	MaterialTextureSlot::Diffuse,			(MetalPlateDir / L"metal_plate_diff_1k.png").Str());
			PrimitiveMaterial.LoadTexture(Device,	CommandList,	MaterialTextureSlot::Normal,			(MetalPlateDir / L"metal_plate_nor_dx_1k.png").Str());
			PrimitiveMaterial.LoadTexture(Device,	CommandList,	MaterialTextureSlot::Specular,			(MetalPlateDir / L"metal_plate_spec_1k.png").Str());
			PrimitiveMaterial.LoadTexture(Device,	CommandList,	MaterialTextureSlot::Metalness,			(MetalPlateDir / L"metal_plate_metal_1k.png").Str());
			PrimitiveMaterial.LoadTexture(Device,	CommandList,	MaterialTextureSlot::Roughness,			(MetalPlateDir / L"metal_plate_rough_1k.png").Str());
			PrimitiveMaterial.LoadTexture(Device,	CommandList,	MaterialTextureSlot::AmbientOcclusion,	(MetalPlateDir / L"metal_plate_ao_1k.png").Str());
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
	 ********** Lights *********
	 ***************************/
	// Place the lights of the scene, they get packed into the lights constant buffer every frame
	{
		// Sun above the scene, shining down from the left and behind
		auto Sun = std::make_unique<DirectionalLight>(TEXT("Sun"));
		Sun->SetDirection({ -0.5f, -1.0f, 0.5f });
		Sun->SetColor({ 1.0f, 0.95f, 0.8f });
		Sun->SetIntensity(0.6f);
		Lights.emplace_back(std::move(Sun));

		// Bulb sitting where the shader used to have its hardcoded light
		auto Bulb = std::make_unique<PointLight>(TEXT("Bulb"));
		Bulb->SetPosition({ 2.0f, 1.0f, -1.0f });
		Bulb->SetColor({ 1.0f, 1.0f, 0.0f });
		Bulb->SetIntensity(1.0f);
		Bulb->SetRange(8.0f);
		Lights.emplace_back(std::move(Bulb));

		// Softbox hanging over the cubes, facing straight down
		auto Softbox = std::make_unique<AreaLight>(TEXT("Softbox"));
		Softbox->SetPosition({ -2.0f, 3.0f, 0.0f });
		Softbox->SetDirection({ 0.0f, -1.0f, 0.0f });
		Softbox->SetSize({ 2.0f, 2.0f });
		Softbox->SetColor({ 0.4f, 0.6f, 1.0f });
		Softbox->SetIntensity(1.0f);
		Softbox->SetRange(12.0f);
		Lights.emplace_back(std::move(Softbox));

		DXASSERT(Lights.size() <= MAX_LIGHTS, "More lights placed in the scene than the constant buffer can hold");
	}

	/***************************
	 ********** Passes *********
	 ***************************/
	// BasePass draws the primitives with their materials into the G-buffer,
	// LightPass then shades that G-buffer with the lights placed above
	SceneGBuffer.Initialize(Device.Get(), WindowWidth, WindowHeight);

	// LightPass samples the G-buffer, a shadow map and two sky cubes out of one heap. This
	// path has neither a shadow pass nor a sky, so those slots get null descriptors and
	// the lights buffer below reports NO_SHADOW_CASTER to skip the lookup entirely.
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc	= {};
		HeapDesc.NumDescriptors				= GBufferTargetCount + 3;
		HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&LightInputsHeap)));

		const auto IncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHandle(LightInputsHeap->GetCPUDescriptorHandleForHeapStart());

		SceneGBuffer.CreateShaderResourceViews(Device.Get(), SrvHandle, IncrementSize);

		// Shadow map slot
		SrvHandle.Offset(GBufferTargetCount, IncrementSize);
		D3D12_SHADER_RESOURCE_VIEW_DESC NullSrvDesc = {};
		NullSrvDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		NullSrvDesc.Format					= DXGI_FORMAT_R32_FLOAT;
		NullSrvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
		NullSrvDesc.Texture2D.MipLevels		= 1;
		Device->CreateShaderResourceView(nullptr, &NullSrvDesc, SrvHandle);

		// Sky cube slots, the dimension has to match what the shader declares
		D3D12_SHADER_RESOURCE_VIEW_DESC NullCubeSrvDesc = {};
		NullCubeSrvDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		NullCubeSrvDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
		NullCubeSrvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURECUBE;
		NullCubeSrvDesc.TextureCube.MipLevels	= 1;

		SrvHandle.Offset(1, IncrementSize);
		Device->CreateShaderResourceView(nullptr, &NullCubeSrvDesc, SrvHandle);

		SrvHandle.Offset(1, IncrementSize);
		Device->CreateShaderResourceView(nullptr, &NullCubeSrvDesc, SrvHandle);
	}
	SceneBasePass.Init(Device.Get(), CommandList.Get());
	SceneLightPass.Init(Device.Get(), CommandList.Get());

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

	// Handle of the depth buffer, BasePass renders against it
	const CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHandle(DsvHeap->GetCPUDescriptorHandleForHeapStart());

	/***************************
	 ********* BasePass ********
	 ***************************/
	// Surface only, every primitive lands in the G-buffer with its material applied and no lighting
	PIXBeginEvent(CommandList.Get(), 0, L"Base Pass");
	{
		SceneGBuffer.TransitionToRenderTarget(CommandList.Get());
		SceneGBuffer.BindAsRenderTarget(CommandList.Get(), DsvHandle);
		SceneGBuffer.Clear(CommandList.Get());

		CommandList->SetGraphicsRootSignature(SceneBasePass.GetRootSignature());
		CommandList->SetPipelineState(SceneBasePass.GetPipelineState());

		// set the descriptor heap shared by every material
		DXDescriptorHeap* MaterialHeaps[] = { SharedMaterialHeap.GetHeap() };
		CommandList->SetDescriptorHeaps(_countof(MaterialHeaps), MaterialHeaps);

		// Plane
		CommandList->SetGraphicsRootDescriptorTable(1, PlaneMaterial.GetDescriptorTable());
		CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + (2 * PrimitiveConstantBufferSize));
		SceneBasePass.Render(CommandList.Get(), PlanePrimitive);

		// Primitives
		CommandList->SetGraphicsRootDescriptorTable(1, PrimitiveMaterial.GetDescriptorTable());
		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			CommandList->SetGraphicsRootConstantBufferView(0, PrimitiveCBV->GetGPUVirtualAddress() + (PrimitiveIdx * PrimitiveConstantBufferSize));
			SceneBasePass.Render(CommandList.Get(), *Primitives[PrimitiveIdx]);
		}
	}
	PIXEndEvent(CommandList.Get());

	/***************************
	 ******** LightPass ********
	 ***************************/
	// Shades what BasePass wrote, straight into the back buffer
	{
		SceneGBuffer.TransitionToShaderResource(CommandList.Get());

		// No depth here, the G-buffer already resolved which surface is visible
		const CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentFrameIdx, RtvIncrementDescriptorSize);
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);

		CommandList->SetGraphicsRootSignature(SceneLightPass.GetRootSignature());
		CommandList->SetPipelineState(SceneLightPass.GetPipelineState());

		// the light pass inputs live in their own heap, so swap it in for the material one
		DXDescriptorHeap* LightHeaps[] = { LightInputsHeap.Get() };
		CommandList->SetDescriptorHeaps(_countof(LightHeaps), LightHeaps);

		CommandList->SetGraphicsRootDescriptorTable(LightPass::GBufferTableIdx,	LightInputsHeap->GetGPUDescriptorHandleForHeapStart());
		CommandList->SetGraphicsRootConstantBufferView(LightPass::CameraCBVIdx,	CameraConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress());
		CommandList->SetGraphicsRootConstantBufferView(LightPass::LightsCBVIdx,	LightsConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress());

		SceneLightPass.RenderFullscreen(CommandList.Get());
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

	// store camera's eye position for the pixel shader
	{
		const auto& EyePosition = Camera.GetPosition();

		// copy camera's position to the mapped constant buffer resource
		memcpy(CameraCbvGPUAddress[CurrentFrameIdx], &EyePosition, sizeof(EyePosition));
	}

	// store every light of the scene for the pixel shader
	{
		LightsConstantBuffer LightsCbv	= {};
		LightsCbv.NumLights				= static_cast<UINT>(Lights.size() < MAX_LIGHTS ? Lights.size() : MAX_LIGHTS);

		for (UINT LightIdx = 0; LightIdx < LightsCbv.NumLights; ++LightIdx)
		{
			LightsCbv.Lights[LightIdx] = Lights[LightIdx]->GetLightConstants();
		}

		// This path has no shadow pass, so nothing casts and the shader skips the lookup
		LightsCbv.ShadowLightIdx = NO_SHADOW_CASTER;

		// copy our LightsConstantBuffer instance to the mapped constant buffer resource
		memcpy(LightsCbvGPUAddress[CurrentFrameIdx], &LightsCbv, sizeof(LightsCbv));
	}

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

		// Normal matrix, see D3D12Math.h. The inverse transpose and the transpose every
		// matrix gets on the way to the GPU cancel, so the plain inverse is stored.
		XMStoreFloat4x4(&CbvPerObject.LocalToWorldNormal, XMMatrixInverse(nullptr, XMLoadFloat4x4(&PlaneScaleRotMatrix)));

		// copy our ConstantBuffer instance to the mapped constant buffer resource
		memcpy(CbvGPUAddress[CurrentFrameIdx] + (2 * PrimitiveConstantBufferSize), &CbvPerObject, sizeof(CbvPerObject));
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
		//Device->Release();
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

void D3D12App::OnMouseMove(int DeltaX, int DeltaY)
{
	Camera.OnMouseMove(DeltaX, DeltaY);
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
	CameraConstantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	CameraCbvGPUAddress.resize(BACK_BUFFER_COUNT);
	LightsConstantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	LightsCbvGPUAddress.resize(BACK_BUFFER_COUNT);

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
			const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PrimitiveConstantBufferSize * 3); // HACK! THREE OBJECTS == 3x constant buffer which is per object!
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

		// Camera CBV Upload heap | One camera per frame, every draw of that frame reads the same one
		{
			// Holds the eye position only, padded up to the constant buffer alignment
			const auto CameraUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			ThrowIfFailed(Device->CreateCommittedResource(
				&DX_HEAP_PROPERTY_UPLOAD,								// this heap will be used to upload the camera constant buffer data
				D3D12_HEAP_FLAG_NONE,									// no flags
				&CameraUploadBufferDesc,								// size of the resource heap
				D3D12_RESOURCE_STATE_GENERIC_READ,						// will be data that is read from so we keep it in the generic read state
				nullptr,												// we do not have use an optimized clear value for constant buffers
				IID_PPV_ARGS(&CameraConstantBufferUploadHeaps[i])));
			NAME_D3D12_OBJECT(CameraConstantBufferUploadHeaps[i], L"Camera Constant Buffer Upload Resource Heap");

			CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
			ThrowIfFailed(CameraConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&CameraCbvGPUAddress[i])));

			const auto& EyePosition = Camera.GetPosition();
			memcpy(CameraCbvGPUAddress[i], &EyePosition, sizeof(EyePosition));										// camera's constant buffer data
		}

		// Lights CBV Upload heap | Every light of the scene, shared by every draw of the frame
		{
			const auto LightsUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(LightsConstantBufferSize);
			ThrowIfFailed(Device->CreateCommittedResource(
				&DX_HEAP_PROPERTY_UPLOAD,								// this heap will be used to upload the lights constant buffer data
				D3D12_HEAP_FLAG_NONE,									// no flags
				&LightsUploadBufferDesc,								// size of the resource heap
				D3D12_RESOURCE_STATE_GENERIC_READ,						// will be data that is read from so we keep it in the generic read state
				nullptr,												// we do not have use an optimized clear value for constant buffers
				IID_PPV_ARGS(&LightsConstantBufferUploadHeaps[i])));
			NAME_D3D12_OBJECT(LightsConstantBufferUploadHeaps[i], L"Lights Constant Buffer Upload Resource Heap");

			CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
			ThrowIfFailed(LightsConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&LightsCbvGPUAddress[i])));

			LightsConstantBuffer LightsCbv = {};
			memcpy(LightsCbvGPUAddress[i], &LightsCbv, sizeof(LightsCbv));											// lights's constant buffer data
		}
	}
}

void D3D12App::InitalizeShaders()
{
	// Shaders defines
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"USE_TEXCOORD", L"1" });

	// Shader paths
	const DXIPATHS::FilePath VertexShaderPath	= DXIPATHS::GetShaderDir / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath	= DXIPATHS::GetShaderDir / L"pixel_shader.hlsl";

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

	VertexShader	= DXIShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PixelShader		= DXIShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
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
		const auto RootCBVDescriptor		= CreateRootDescriptor(0, 0);	// b0 | Per object
		const auto RootCameraCBVDescriptor	= CreateRootDescriptor(0, 1);	// b1 | Per camera
		const auto RootLightsCBVDescriptor	= CreateRootDescriptor(0, 2);	// b2 | Per lights of the scene

		// create a descriptor table
		// create a descriptor range (descriptor table) and fill it out
		// this is a range of descriptors inside a descriptor heap
		// One range covering every texture slot of a material, so binding a material is a single call
		const auto DescriptorRange = CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MaterialTextureSlotCount, 0, 0);
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

		// Camera CBV | Pixel shader reads the eye position out of it for specular lighting
		RootParamHelper ParamCameraCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
								D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
								RootCameraCBVDescriptor);

		// Lights CBV | Pixel shader shades with every light of the scene out of it
		RootParamHelper ParamLightsCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
								D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
								RootLightsCBVDescriptor);

		// Create root parameters array | Keep the order, Render() binds them by index
		auto RootParameters = CreateRootParameters({ ParamCBV, ParamSRV, ParamCameraCBV, ParamLightsCBV });

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
		RootSignatureDesc.Init(	RootParameters.size(), // we have 4 root parameters
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
	const CD3DX12_RESOURCE_BARRIER BarrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &BarrierPresentToRTV);

	// Set necessary state.
	// NOTE | Each pass binds its own root signature, pipeline state and render targets in Render()
	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHandle(DsvHeap->GetCPUDescriptorHandleForHeapStart());
	CommandList->ClearDepthStencilView(DsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
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

	// Normal matrix, see D3D12Math.h. The inverse transpose and the transpose every matrix
	// gets on the way to the GPU cancel, so the plain inverse is what is stored.
	XMStoreFloat4x4(&PrimitiveCBVData.LocalToWorldNormal, XMMatrixInverse(nullptr, LocalToWorld));

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(PrimitivesConstantBufferPtr + (PrimitiveIdx * PrimitiveConstantBufferSize), &PrimitiveCBVData, PrimitiveConstantBufferSize /* size of */);
}