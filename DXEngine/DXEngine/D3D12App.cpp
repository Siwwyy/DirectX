#include "D3D12App.h"


#include "Win32Proc.h"
#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Utils.h"



//CONSTANTS
_CONSTEVAL D3D12_COMMAND_LIST_TYPE			COMMAND_LIST_TYPE			= D3D12_COMMAND_LIST_TYPE_DIRECT;
_CONSTEVAL UINT								BACK_BUFFER_COUNT			= 3;
_CONSTEVAL D3D_FEATURE_LEVEL				D3D12_FEATURE_LEVEL			= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
_CONSTEVAL DXGI_FORMAT						BACK_BUFFER_FORMAT			= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
_CONSTEVAL DXGI_FORMAT						DEPTH_STENCIL_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format

// HEAP PROPERTIES
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_DEFAULT		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);		//Heap type default
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_UPLOAD		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		//Heap type upload
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_READBACK		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);	//Heap type readback
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_CUSTOM		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_CUSTOM);		//Heap type custom

//Globals


// Vertices/Index buffers etc.
Vertex CubeVertices[] =
{
	// front face
	{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	// right side face
	{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	// left side face
	{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	// back face
	{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	// top face
	{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

	// bottom face
	{  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },


	/////////////////////////////
	//{ -0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
	//{  0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	//{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
	//{  0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f }
	/////////////////////////////

};

constexpr UINT VertexBufferSize = sizeof(CubeVertices);

DWORD CubeIndices[] = {
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

constexpr UINT IndexBufferSize = sizeof(CubeIndices);
constexpr UINT NumCubeIndices = IndexBufferSize / sizeof(DWORD);

//////////////////////////////////////////////////////////////////

using namespace Helpers;
using namespace Utils;

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

	//Create dxgi factory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

	// Initialize device
	const ComPtr<IDXGIAdapter1> hardwareAdapter = GetAdapter(Factory, D3D12_FEATURE_LEVEL);
	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&Device)
	));


	// Initialization of command queue
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0; //single GPU env for now
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type = COMMAND_LIST_TYPE;
	CommandQueue = Utils::CreateCommandQueue(Device.Get(), CommandQueueDesc);

	// Initialization of Swap Chain
	// Get aplication window's handle (hwnd)
	auto windowHwnd = Win32Proc::GetHwnd();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc		= {};
	swapChainDesc.BufferCount				= BACK_BUFFER_COUNT;
	swapChainDesc.Width						= WindowWidth;
	swapChainDesc.Height					= WindowHeight;
	swapChainDesc.Format					= BACK_BUFFER_FORMAT;
	swapChainDesc.BufferUsage				= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect				= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.SwapEffect				= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.SampleDesc.Count			= 1;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(Factory->CreateSwapChainForHwnd(
		CommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		windowHwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	//auto swapChain1 = dynamic_cast<IDXGISwapChain1*>(SwapChain.Get()); //retrieve IDXGISwapChain1 from IDXGISwapChain3

	// Convert SwapChain1 interface to SwapChain4
	ThrowIfFailed(swapChain1.As(&SwapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(Factory->MakeWindowAssociation(windowHwnd, DXGI_MWA_NO_ALT_ENTER));
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Create RTV Descriptor Heaps
	constexpr auto descriptorHeapType	= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto descriptorHeapFlags	= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc	= {};
	rtvHeapDesc.NumDescriptors				= BACK_BUFFER_COUNT;
	rtvHeapDesc.Type						= descriptorHeapType;
	rtvHeapDesc.Flags						= descriptorHeapFlags;
	ThrowIfFailed(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	// Get Render Target View Increment Descriptor size
	RtvIncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(descriptorHeapType);

	// Create an event handle to use for frame synchronization.
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	//Initialize resources per frame buffer
	InitializePerFrameResources();

	//// Root parameters
	//// create a root descriptor, which explains where to find the data for this root parameter
	//D3D12_ROOT_DESCRIPTOR RootCBVDescriptor = CreateRootDescriptor(0, 0);

	//// Create root parameters
	//const std::vector<D3D12_ROOT_PARAMETER> RootParameters = CreateRootParameters(
	//	{
	//		RootParamHelper(D3D12_ROOT_PARAMETER_TYPE_CBV, D3D12_SHADER_VISIBILITY_VERTEX, RootCBVDescriptor),
	//	}
	//);

	// create a root descriptor, which explains where to find the data for this root parameter
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// create a root parameter and fill it out
	D3D12_ROOT_PARAMETER  rootParameters[1]; // only one parameter right now
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now


	// Create Root Signature Descriptor
	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
	RootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
		rootParameters, // a pointer to the beginning of our root parameters array
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

	//Shaders compilation //TODO
	// Shaders defines
	constexpr DxcDefine shaderDefines[] =
	{
		L"PIXEL_SHADER_ENABLED", L"1",
		NULL, NULL
	};

	constexpr const wchar_t* vertexShaderPath = L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//vertex_shader.hlsl";
	constexpr const wchar_t* pixelShaderPath = L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//pixel_shader.hlsl";

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
	VertexShader = shaderCompiler.CompileShader(vertexShaderPath, nullptr, L"VSMain", L"vs_6_0");
	PixelShader = shaderCompiler.CompileShader(pixelShaderPath, shaderDefines, L"PSMain", L"ps_6_0");

	// Pipeline state object (PSO)
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	//psoDesc.DepthStencilState.DepthEnable	= FALSE;
	//psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState					= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat							= DEPTH_STENCIL_FORMAT;
	psoDesc.SampleMask							= UINT_MAX;
	psoDesc.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets					= 1;
	psoDesc.RTVFormats[0]						= BACK_BUFFER_FORMAT;
	psoDesc.SampleDesc.Count					= 1;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineState)));

	//Initialization of command list
	CommandList = Utils::CreateGraphicsCommandList(
		Device.Get(),
		CommandListDesc{ 0, COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAG_NONE },
		CommandAllocators[0].Get(),
		PipelineState.Get());

	ThrowIfFailed(CommandList->Close()); //close not closed command list
	// Reset previously used command list and command allocator
	ThrowIfFailed(CommandAllocators[0]->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocators[0].Get(), PipelineState.Get()));

	// Create vertex buffer
	const auto VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY_DEFAULT,								// a default heap
		D3D12_HEAP_FLAG_NONE,								// no flags
		&VertexBufferDesc,									// resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST,						// we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
		nullptr,											// optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&VertexBuffer)));
	NAME_D3D12_OBJECT(VertexBuffer, "VertexBuffer");

	// create upload heap
	// upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
	// We will upload the vertex buffer using this heap to the default heap
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY_UPLOAD,								// upload heap
		D3D12_HEAP_FLAG_NONE,								// no flags
		&VertexBufferDesc,									// resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ,					// GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&VertexBufferUploadHeap)));
	NAME_D3D12_OBJECT(VertexBufferUploadHeap, "Vertex Buffer Upload Resource Heap");

	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA VertexData = {};
	VertexData.pData = reinterpret_cast<UINT8*>(CubeVertices);
	VertexData.RowPitch = VertexBufferSize;
	VertexData.SlicePitch = VertexData.RowPitch;

	UpdateSubresources<1>(CommandList.Get(), VertexBuffer.Get(), VertexBufferUploadHeap.Get(), 0, 0, 1, &VertexData);
	// transition the vertex buffer data from copy destination state to vertex buffer state
	const auto VertexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	CommandList->ResourceBarrier(1, &VertexCmdListBarrier);

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes = sizeof(Vertex);
	VertexBufferView.SizeInBytes = sizeof(CubeVertices);

	// Index Buffer
	// Create index buffer
	// create default heap to hold index buffer
	const auto IndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY_DEFAULT,								// a default heap
		D3D12_HEAP_FLAG_NONE,								// no flags
		&IndexBufferDesc,									// resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST,						// start in the copy destination state
		nullptr,											// optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&IndexBuffer)));

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	NAME_D3D12_OBJECT(IndexBuffer, "Index Buffer Resource");

	// create upload heap to upload index buffer
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY_UPLOAD,								// upload heap
		D3D12_HEAP_FLAG_NONE,								// no flags
		&IndexBufferDesc,									// resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ,					// GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&IndexBufferUploadHeap)));
	NAME_D3D12_OBJECT(IndexBufferUploadHeap, "Index Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA IndexData = {};
	IndexData.pData = reinterpret_cast<UINT8*>(CubeIndices); // pointer to our index array
	IndexData.RowPitch = IndexBufferSize; // size of all our index buffer
	IndexData.SlicePitch = IndexData.RowPitch; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources<1>(CommandList.Get(), IndexBuffer.Get(), IndexBufferUploadHeap.Get(), 0, 0, 1, &IndexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	const auto IndexBufferCmdBarrier = CD3DX12_RESOURCE_BARRIER::Transition(IndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	CommandList->ResourceBarrier(1, &IndexBufferCmdBarrier);

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
	IndexBufferView.Format			= DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	IndexBufferView.SizeInBytes		= IndexBufferSize;

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
	const auto depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(DEPTH_STENCIL_FORMAT, WindowWidth, WindowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY_DEFAULT,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
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


	// Create View, Projection Matrices
	{
		// build projection and view matrix
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)WindowWidth / (float)WindowHeight, 0.1f, 1000.0f);
		XMStoreFloat4x4(&CamMatrices.cameraProjMat, tmpMat);

		// set starting camera state
		CamMatrices.cameraPosition		= DirectX::XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
		CamMatrices.cameraTarget		= DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		CamMatrices.cameraUp			= DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

		// build view matrix
		DirectX::XMVECTOR cPos			= DirectX::XMLoadFloat4(&CamMatrices.cameraPosition);
		DirectX::XMVECTOR cTarg			= DirectX::XMLoadFloat4(&CamMatrices.cameraTarget);
		DirectX::XMVECTOR cUp			= DirectX::XMLoadFloat4(&CamMatrices.cameraUp);
		tmpMat							= DirectX::XMMatrixLookAtLH(cPos, cTarg, cUp);
		XMStoreFloat4x4(&CamMatrices.cameraViewMat, tmpMat);

		// set starting cubes position
		// first cube
		Cube1Matrices.cubePosition		= DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
		DirectX::XMVECTOR posVec		= DirectX::XMLoadFloat4(&Cube1Matrices.cubePosition); // create xmvector for cube1's position
		tmpMat							= DirectX::XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
		DirectX::XMStoreFloat4x4(&Cube1Matrices.cubeRotMat, DirectX::XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
		DirectX::XMStoreFloat4x4(&Cube1Matrices.cubeWorldMat, tmpMat); // store cube1's world matrix

		// second cube
		Cube2Matrices.cubePosition		= DirectX::XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
		posVec							= DirectX::XMLoadFloat4(&Cube2Matrices.cubePosition) + DirectX::XMLoadFloat4(&Cube1Matrices.cubePosition);
		// create xmvector for cube2's position, we are rotating around cube1 here, so add cube2's position to cube1
		tmpMat							= DirectX::XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
		DirectX::XMStoreFloat4x4(&Cube2Matrices.cubeRotMat, DirectX::XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
		DirectX::XMStoreFloat4x4(&Cube2Matrices.cubeWorldMat, tmpMat); // store cube2's world matrix

		//// build projection and view matrix
		//XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)WindowWidth / (float)WindowHeight, 0.1f, 1000.0f);
		//XMStoreFloat4x4(&CamMatrices.cameraProjMat, tmpMat);

		//// set starting camera state
		//CamMatrices.cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
		//CamMatrices.cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		//CamMatrices.cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

		//// build view matrix
		//XMVECTOR cPos = XMLoadFloat4(&CamMatrices.cameraPosition);
		//XMVECTOR cTarg = XMLoadFloat4(&CamMatrices.cameraTarget);
		//XMVECTOR cUp = XMLoadFloat4(&CamMatrices.cameraUp);
		//tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
		//XMStoreFloat4x4(&CamMatrices.cameraViewMat, tmpMat);

		//// set starting cubes position
		//// first cube
		//Cube1Matrices.cubePosition = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
		//XMVECTOR posVec = XMLoadFloat4(&Cube1Matrices.cubePosition); // create xmvector for cube1's position

		//tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
		//XMStoreFloat4x4(&Cube1Matrices.cubeRotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
		//XMStoreFloat4x4(&Cube1Matrices.cubeWorldMat, tmpMat); // store cube1's world matrix

		//// second cube
		//Cube2Matrices.cubePosition = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
		//posVec = XMLoadFloat4(&Cube2Matrices.cubePosition) + XMLoadFloat4(&Cube1Matrices.cubePosition); // create xmvector for cube2's position
		//																			// we are rotating around cube1 here, so add cube2's position to cube1

		//tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
		//XMStoreFloat4x4(&Cube2Matrices.cubeRotMat, XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
		//XMStoreFloat4x4(&Cube2Matrices.cubeWorldMat, tmpMat); // store cube2's world matrix
	}
}

void D3D12App::Render()
{
	//Always BeginFrame first
	BeginFrame();

	// Set necessary state.
	CommandList->SetGraphicsRootSignature(RootSignature.Get());
	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);	
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CommandList->IASetIndexBuffer(&IndexBufferView);

	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER barrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &barrierPresentToRTV);

	constexpr float clearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentFrameIdx, RtvIncrementDescriptorSize);
	CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(DsvHeap->GetCPUDescriptorHandleForHeapStart());
	CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// first cube

	// set cube1's constant buffer
	CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress());

	// draw first cube
	CommandList->DrawIndexedInstanced(NumCubeIndices, 1, 0, 0, 0);

	// second cube

	// set cube2's constant buffer. You can see we are adding the size of ConstantBufferPerObject to the constant buffer
	// resource heaps address. This is because cube1's constant buffer is stored at the beginning of the resource heap, while
	// cube2's constant buffer data is stored after (256 bits from the start of the heap).
	CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + ConstantBufferPerObjectSize);

	// draw second cube
	CommandList->DrawIndexedInstanced(NumCubeIndices, 1, 0, 0, 0);

	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER barrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &barrierRTVtoPresent);

	//Always EndFrame last
	EndFrame();
}

void D3D12App::Update()
{
	// update app logic, such as moving the camera or figuring out what objects are in view

	// create rotation matrices
	XMMATRIX rotXMat	= XMMatrixRotationX(0.0001f);
	XMMATRIX rotYMat	= XMMatrixRotationY(0.0002f);
	XMMATRIX rotZMat	= XMMatrixRotationZ(0.0003f);

	// add rotation to cube1's rotation matrix and store it
	XMMATRIX rotMat		= XMLoadFloat4x4(&Cube1Matrices.cubeRotMat) * rotXMat * rotYMat * rotZMat;
	XMStoreFloat4x4(&Cube1Matrices.cubeRotMat, rotMat);

	// create translation matrix for cube 1 from cube 1's position vector
	XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&Cube1Matrices.cubePosition));

	// create cube1's world matrix by first rotating the cube, then positioning the rotated cube
	XMMATRIX worldMat		= rotMat * translationMat;

	// store cube1's world matrix
	XMStoreFloat4x4(&Cube1Matrices.cubeWorldMat, worldMat);

	// update constant buffer for cube1
	// create the wvp matrix and store in constant buffer
	XMMATRIX viewMat		= XMLoadFloat4x4(&CamMatrices.cameraViewMat); // load view matrix
	XMMATRIX projMat		= XMLoadFloat4x4(&CamMatrices.cameraProjMat); // load projection matrix
	XMMATRIX wvpMat			= XMLoadFloat4x4(&Cube1Matrices.cubeWorldMat) * viewMat * projMat; // create wvp matrix
	XMMATRIX transposed		= XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerCube.WorldViewProjectionMat4x4, transposed); // store transposed wvp matrix in constant buffer
	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx], &CbvPerCube, sizeof(CbvPerCube));

	// now do cube2's world matrix
	// create rotation matrices for cube2
	rotXMat = XMMatrixRotationX(0.0003f);
	rotYMat = XMMatrixRotationY(0.0002f);
	rotZMat = XMMatrixRotationZ(0.0001f);

	// add rotation to cube2's rotation matrix and store it
	rotMat = rotZMat * (XMLoadFloat4x4(&Cube2Matrices.cubeRotMat) * (rotXMat * rotYMat));
	XMStoreFloat4x4(&Cube2Matrices.cubeRotMat, rotMat);

	// create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
	XMMATRIX translationOffsetMat	= XMMatrixTranslationFromVector(XMLoadFloat4(&Cube2Matrices.cubePosition));

	// we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
	XMMATRIX scaleMat				= XMMatrixScaling(0.5f, 0.5f, 0.5f);

	// reuse worldMat. 
	// first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
	// then we translate it. 
	// then we rotate it. rotation always rotates around point 0,0,0
	// finally we move it to cube 1's position, which will cause it to rotate around cube 1
	worldMat		= scaleMat * translationOffsetMat * rotMat * translationMat;
	wvpMat			= XMLoadFloat4x4(&Cube2Matrices.cubeWorldMat) * viewMat * projMat; // create wvp matrix
	transposed		= XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&CbvPerCube.WorldViewProjectionMat4x4, transposed); // store transposed wvp matrix in constant buffer
	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(CbvGPUAddress[CurrentFrameIdx] + ConstantBufferPerObjectSize, &CbvPerCube, sizeof(CbvPerCube));

	// store cube2's world matrix
	XMStoreFloat4x4(&Cube2Matrices.cubeWorldMat, worldMat);

	//	// update app logic, such as moving the camera or figuring out what objects are in view

	//// create rotation matrices
	//XMMATRIX rotXMat = XMMatrixRotationX(0.0001f);
	//XMMATRIX rotYMat = XMMatrixRotationY(0.0002f);
	//XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f);

	//// add rotation to cube1's rotation matrix and store it
	//XMMATRIX rotMat = XMLoadFloat4x4(&Cube1Matrices.cubeRotMat) * rotXMat * rotYMat * rotZMat;
	//XMStoreFloat4x4(&Cube1Matrices.cubeRotMat, rotMat);

	//// create translation matrix for cube 1 from cube 1's position vector
	//XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&Cube1Matrices.cubePosition));

	//// create cube1's world matrix by first rotating the cube, then positioning the rotated cube
	//XMMATRIX worldMat = rotMat * translationMat;

	//// store cube1's world matrix
	//XMStoreFloat4x4(&Cube1Matrices.cubeWorldMat, worldMat);

	//// update constant buffer for cube1
	//// create the wvp matrix and store in constant buffer
	//XMMATRIX viewMat = XMLoadFloat4x4(&CamMatrices.cameraViewMat); // load view matrix
	//XMMATRIX projMat = XMLoadFloat4x4(&CamMatrices.cameraProjMat); // load projection matrix
	//XMMATRIX wvpMat = XMLoadFloat4x4(&Cube1Matrices.cubeWorldMat) * viewMat * projMat; // create wvp matrix
	//XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	//XMStoreFloat4x4(&CbvPerCube.WorldViewProjectionMat4x4, transposed); // store transposed wvp matrix in constant buffer

	//// copy our ConstantBuffer instance to the mapped constant buffer resource
	//memcpy(CbvGPUAddress[CurrentFrameIdx], &CbvPerCube, sizeof(CbvPerCube));

	//// now do cube2's world matrix
	//// create rotation matrices for cube2
	//rotXMat = XMMatrixRotationX(0.0003f);
	//rotYMat = XMMatrixRotationY(0.0002f);
	//rotZMat = XMMatrixRotationZ(0.0001f);

	//// add rotation to cube2's rotation matrix and store it
	//rotMat = rotZMat * (XMLoadFloat4x4(&Cube2Matrices.cubeRotMat) * (rotXMat * rotYMat));
	//XMStoreFloat4x4(&Cube2Matrices.cubeRotMat, rotMat);

	//// create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
	//XMMATRIX translationOffsetMat = XMMatrixTranslationFromVector(XMLoadFloat4(&Cube2Matrices.cubePosition));

	//// we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
	//XMMATRIX scaleMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);

	//// reuse worldMat. 
	//// first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
	//// then we translate it. 
	//// then we rotate it. rotation always rotates around point 0,0,0
	//// finally we move it to cube 1's position, which will cause it to rotate around cube 1
	//worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

	//wvpMat = XMLoadFloat4x4(&Cube2Matrices.cubeWorldMat) * viewMat * projMat; // create wvp matrix
	//transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	//XMStoreFloat4x4(&CbvPerCube.WorldViewProjectionMat4x4, transposed); // store transposed wvp matrix in constant buffer

	//// copy our ConstantBuffer instance to the mapped constant buffer resource
	//memcpy(CbvGPUAddress[CurrentFrameIdx] + ConstantBufferPerObjectAlignedSize, &CbvPerCube, sizeof(CbvPerCube));

	//// store cube2's world matrix
	//XMStoreFloat4x4(&Cube2Matrices.cubeWorldMat, worldMat);
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
	// Constant buffer heaps
	ConstantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	// Constat buffer gpu addresses
	CbvGPUAddress.resize(BACK_BUFFER_COUNT);

	// Get handle for 0th descriptor in heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());

	//
	const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
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
			Device->CreateRenderTargetView(RenderTargets[i].Get(), nullptr, rtvHandle);

			// Offsets the rtv handle by descriptor size -> new_rtvHandle = rtvHandle + 1 * rtvDescriptorSize
			rtvHandle.Offset(1, RtvIncrementDescriptorSize);
		}

		// CBV Upload heap
		{
			// create resource for cubes
			ThrowIfFailed(Device->CreateCommittedResource(
				&HEAP_PROPERTY_UPLOAD,									// this heap will be used to upload the constant buffer data
				D3D12_HEAP_FLAG_NONE,									// no flags
				&UploadBufferDesc,										// size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
				D3D12_RESOURCE_STATE_GENERIC_READ,						// will be data that is read from so we keep it in the generic read state
				nullptr,												// we do not have use an optimized clear value for constant buffers
				IID_PPV_ARGS(&ConstantBufferUploadHeaps[i])));
			NAME_D3D12_OBJECT(ConstantBufferUploadHeaps[i], "Constant Buffer Upload Resource Heap");
			ZeroMemory(&CbvPerCube, sizeof(CbvPerCube));

			CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
			// map the resource heap to get a gpu virtual address to the beginning of the heap
			ThrowIfFailed(ConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&CbvGPUAddress[i])));

			// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
			// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
			memcpy(CbvGPUAddress[i], &CbvPerCube, sizeof(CbvPerCube));											// cube1's constant buffer data
			memcpy(CbvGPUAddress[i] + ConstantBufferPerObjectSize, &CbvPerCube, sizeof(CbvPerCube));	// cube2's constant buffer data
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
}

void D3D12App::EndFrame()
{
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