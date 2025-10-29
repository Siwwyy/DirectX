#include "D3D12App.h"


// Own Includes
#include "Win32Proc.h"
#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"

// Namespaces
using namespace Helpers;


//CONSTANTS
const D3D12_COMMAND_LIST_TYPE			COMMAND_LIST_TYPE			= D3D12_COMMAND_LIST_TYPE_DIRECT;
const UINT								BACK_BUFFER_COUNT			= 3;
const D3D_FEATURE_LEVEL					D3D12_FEATURE_LEVEL			= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
const DXGI_FORMAT						BACK_BUFFER_FORMAT			= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
const DXGI_FORMAT						DEPTH_STENCIL_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format



//Globals


// Vertices/Index buffers etc.
Vertex CubeVertices[] =
{
//	// front face
//	{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//	{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//	// right side face
//	{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//	{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//	// left side face
//	{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//	{ -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//	// back face
//	{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//	{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//	// top face
//	{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//	{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//	// bottom face
//	{  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//	{  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f }
//
//
//
//	/////////////////////////////
//	//{ -0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//	//{  0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//	//{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//	//{  0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f }
//	/////////////////////////////
//
	//With UV Tex coord
	// front face
	{ -0.5f, 0.5f, -0.5f, 0.0f, 0.0f },
	{ 0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
	{ -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
	{ 0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

	// right side face
	{ 0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
	{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
	{ 0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
	{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

	// left side face
	{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
	{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

	// back face
	{ 0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
	{ -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
	{ 0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

	// top face
	{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
	{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
	{ 0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

	// bottom face
	{ 0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
	{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
	{ 0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
 
};

constexpr UINT VertexBufferSize = sizeof(CubeVertices);

DWORD CubeIndices[] = 
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

	///////////////////////////////
	//0, 2, 1,
	//0, 3, 1

	///////////////////////////////
};

constexpr UINT IndexBufferSize	= sizeof(CubeIndices);
constexpr UINT NumCubeIndices	= IndexBufferSize / sizeof(DWORD);

//////////////////////////////////////////////////////////////////



D3D12App::D3D12App(const UINT windowWidth, const UINT windowHeight, const std::wstring windowName)
	: WindowWidth(windowWidth)
	, WindowHeight(windowHeight)
	, AspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight))
	, WindowName(windowName)
	, ViewPort(0.f, 0.f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 1.0f)
	, ScissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight))
	, CurrentFrameIdx(0)
	, RtvIncrementDescriptorSize(0)
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");
}

void D3D12App::Initialize()
{
	// Initialize factory
	UINT DxgiFactoryFlags = 0;

#if DEBUG_MODE
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> DebugController;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DebugController))))
		{
			DebugController->EnableDebugLayer();

			// Enable additional debug layers.
			DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	// Sample Descriptor
	DXGI_SAMPLE_DESC SAMPLE_DESC = {};
	SAMPLE_DESC.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

	//Create dxgi factory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

	// Initialize device
	const ComPtr<IDXGIAdapter1> HardwareAdapter = GetAdapter(Factory, D3D12_FEATURE_LEVEL);
	ThrowIfFailed(D3D12CreateDevice(
		HardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&Device)
	));


	// Initialization of command queue
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0; //single GPU env for now
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type = COMMAND_LIST_TYPE;
	CommandQueue = Helpers::CreateCommandQueue(Device.Get(), CommandQueueDesc);

	// Initialization of Swap Chain
	// Get aplication window's handle (hwnd)
	auto windowHwnd = Win32Proc::GetHwnd();

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
		windowHwnd,
		&SwapChainDesc,
		nullptr,
		nullptr,
		&SwapChain1
	));

	//auto SwapChain1 = dynamic_cast<IDXGISwapChain1*>(SwapChain.Get()); //retrieve IDXGISwapChain1 from IDXGISwapChain3

	// Convert SwapChain1 interface to SwapChain4
	ThrowIfFailed(SwapChain1.As(&SwapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(Factory->MakeWindowAssociation(windowHwnd, DXGI_MWA_NO_ALT_ENTER));
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Create RTV Descriptor Heaps
	constexpr auto DescriptorHeapFlags	= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto descriptorHeapFlags	= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc	= {};
	rtvHeapDesc.NumDescriptors				= BACK_BUFFER_COUNT;
	rtvHeapDesc.Type						= DescriptorHeapFlags;
	rtvHeapDesc.Flags						= descriptorHeapFlags;
	ThrowIfFailed(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	// Get Render Target View Increment Descriptor size
	RtvIncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(DescriptorHeapFlags);

	// Create an event handle to use for frame synchronization.
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	//Initialize resources per frame buffer
	InitializePerFrameResources();

	// Root parameters
	// create a root descriptor, which explains where to find the data for this root parameter
	D3D12_ROOT_DESCRIPTOR RootCBVDescriptor = CreateRootDescriptor(0, 0);

	// create a descriptor range (descriptor table) and fill it out
	// this is a range of descriptors inside a descriptor heap
	D3D12_DESCRIPTOR_RANGE  DescriptorTableRanges[1]; // only one range right now
	DescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
	DescriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
	DescriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	DescriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	DescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables
	D3D12_ROOT_DESCRIPTOR_TABLE RootSRV_DescriptorTable = CreateRootDescriptorTable(_countof(DescriptorTableRanges), DescriptorTableRanges);

	// Create root parameters
	const std::vector<D3D12_ROOT_PARAMETER> RootParameters = CreateRootParameters({
			RootParamHelper(D3D12_ROOT_PARAMETER_TYPE_CBV, D3D12_SHADER_VISIBILITY_VERTEX, RootCBVDescriptor),
			RootParamHelper(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, D3D12_SHADER_VISIBILITY_PIXEL, RootSRV_DescriptorTable)
	});

	// create a static sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create Root Signature Descriptor
	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Init(RootParameters.size(), // we have 1 root parameter
		RootParameters.data(), // a pointer to the beginning of our root parameters array
		1, // we have one static sampler
		&sampler, // a pointer to our static sampler (array)
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

	//Shaders compilation //TODO
	// Shaders defines
	constexpr DxcDefine shaderDefines[] =
	{
		L"PIXEL_SHADER_ENABLED", L"1",
		NULL, NULL
	};

	constexpr const wchar_t* VertexShaderPath = L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//vertex_shader.hlsl";
	constexpr const wchar_t* PixelShaderPath = L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;

	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS); //-Od
	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
	arguments.push_back(DXC_ARG_DEBUG); //-Zi


	for (const auto& define : shaderDefines)
	{
		arguments.push_back(L"-D");
		arguments.push_back(define.Name);
		arguments.push_back(L"=");
		arguments.push_back(define.Value);
	}

	D3D12ShaderCompiler shaderCompiler;
	VertexShader = shaderCompiler.CompileShader(VertexShaderPath, nullptr, L"VSMain", L"vs_6_0");
	PixelShader = shaderCompiler.CompileShader(PixelShaderPath, shaderDefines, L"PSMain", L"ps_6_0");

	// Pipeline state object (PSO)
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		//{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	inputLayoutDesc.NumElements = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputElementDescs;

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc	= {};
	psoDesc.InputLayout							= inputLayoutDesc;
	psoDesc.pRootSignature						= RootSignature.Get();
	psoDesc.VS									= CD3DX12_SHADER_BYTECODE(VertexShader.Get());
	psoDesc.PS									= CD3DX12_SHADER_BYTECODE(PixelShader.Get());
	psoDesc.RasterizerState						= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState							= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState					= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat							= DEPTH_STENCIL_FORMAT;
	psoDesc.SampleMask							= UINT_MAX;
	psoDesc.SampleDesc							= SAMPLE_DESC;
	psoDesc.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets					= 1;
	psoDesc.RTVFormats[0]						= BACK_BUFFER_FORMAT;
	psoDesc.SampleDesc.Count					= 1;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineState)));

	//Initialization of command list
	CommandList = Helpers::CreateGraphicsCommandList(
		Device.Get(),
		CommandListDesc{ 0, COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAG_NONE },
		CommandAllocators[0].Get(),
		PipelineState.Get());

	ThrowIfFailed(CommandList->Close()); //close not closed command list
	// Reset previously used command list and command allocator
	ThrowIfFailed(CommandAllocators[0]->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocators[0].Get(), PipelineState.Get()));

	//Depth Stencil
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc	= {};
	DsvHeapDesc.NumDescriptors				= 1;
	DsvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
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

	//Get Depth/Stencil View Descriptor Incremental Size (to get e.g., next resource/desc in the heap)
	DsvIncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//Submit necessary things from command list
	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close command list for execution
	DXCommandList* commandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// Move Fence / Wait for previous frame to end
	WaitForPreviousFrame();
}

void D3D12App::Render()
{
	//Always BeginFrame first
	BeginFrame();




	//Always EndFrame last
	EndFrame();
}

void D3D12App::Update(float DeltaTime)
{
	
}

void D3D12App::Destroy()
{

}

void D3D12App::InitializePerFrameResources()
{
	DXASSERT(Device && SwapChain, "Device can not be null for Initialization per frame resources");

	// Resize arrays/vectors per frame buffer resources
	// Fences
	Fences.resize(BACK_BUFFER_COUNT);
	// Fence Values
	FenceValue.resize(BACK_BUFFER_COUNT);
	// Command allocators
	CommandAllocators.resize(BACK_BUFFER_COUNT);
	// Render Targets
	RenderTargets.resize(BACK_BUFFER_COUNT);


	// Get handle for 0th descriptor in heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());

	//
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
	}
}

void D3D12App::BeginFrame()
{
	// Get Current Frame index 
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Reset previously used command list and command allocator
	ThrowIfFailed(CommandAllocators[CurrentFrameIdx]->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocators[CurrentFrameIdx].Get(), PipelineState.Get()));

	// Set necessary state.
	CommandList->SetGraphicsRootSignature(RootSignature.Get());
	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER BarrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &BarrierPresentToRTV);
}

void D3D12App::EndFrame()
{
	constexpr float ClearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	const CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentFrameIdx, RtvIncrementDescriptorSize);
	CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
	CommandList->ClearRenderTargetView(RtvHandle, ClearColor, 0, nullptr);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHandle(DsvHeap->GetCPUDescriptorHandleForHeapStart());
	CommandList->ClearDepthStencilView(DsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER BarrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &BarrierRTVtoPresent);

	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close command list for execution
	DXCommandList* commandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// Present the frame
	ThrowIfFailed(SwapChain->Present(1, 0));

	// Move Fence / Wait for previous frame to end
	WaitForPreviousFrame();
}

void D3D12App::WaitForPreviousFrame()
{
	// Signal and increment the fence value.
	FenceValue[CurrentFrameIdx]++;
	ThrowIfFailed(CommandQueue->Signal(Fences[CurrentFrameIdx].Get(), FenceValue[CurrentFrameIdx]));

	// Wait until recorded commands are processed by GPU.
	if (Fences[CurrentFrameIdx]->GetCompletedValue() < FenceValue[CurrentFrameIdx])
	{
		ThrowIfFailed(Fences[CurrentFrameIdx]->SetEventOnCompletion(FenceValue[CurrentFrameIdx], FenceEvent));
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}