#include "D3D12App.h"


#include "Win32Proc.h"
#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Utils.h"



//CONSTANTS
_CONSTEVAL D3D12_COMMAND_LIST_TYPE		COMMAND_LIST_TYPE				= D3D12_COMMAND_LIST_TYPE_DIRECT;
_CONSTEVAL UINT							BACK_BUFFER_COUNT				= 2;
_CONSTEVAL D3D_FEATURE_LEVEL			D3D12_FEATURE_LEVEL				= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
_CONSTEVAL DXGI_FORMAT					BACK_BUFFER_FORMAT				= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
_CONSTEVAL DXGI_FORMAT					DEPTH_STENCIL_FORMAT			= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;		//depth stencil format

// HEAP PROPERTIES
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_DEFAULT		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);		//Heap type default
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_UPLOAD		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		//Heap type upload
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_READBACK		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);	//Heap type readback
static const CD3DX12_HEAP_PROPERTIES		HEAP_PROPERTY_CUSTOM		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_CUSTOM);		//Heap type custom



using namespace Helpers;
//using namespace Math;
using namespace Utils;


D3D12App::D3D12App(const UINT windowWidth, const UINT windowHeight, const std::wstring windowName)
	: windowWidth(windowWidth)
	, windowHeight(windowHeight)
	, aspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight))
	, windowName(windowName)
	, viewPort(0.f, 0.f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 1.0f)
	, scissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight))
	, bufferCount(BACK_BUFFER_COUNT)
	, currentFrameIdx(0)
	, rtvIncrementDescriptorSize(0)
	, camMatrices({})
	, cube1Matrices({})
	, cube2Matrices({})
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");

	// Render Targets
	renderTargets.resize(BACK_BUFFER_COUNT);
	// Constant buffer heaps
	constantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	// Constat buffer gpu addresses
	cbvGPUAddress.resize(BACK_BUFFER_COUNT);
}

void D3D12App::Initialize()
{

	// Initialize factory
	UINT dxgiFactoryFlags = 0;

#if DEBUG_MODE
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	// Initialize device
	const ComPtr<IDXGIAdapter1> hardwareAdapter = GetAdapter(factory, D3D12_FEATURE_LEVEL);
	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&device)
	));


	// Initialization of command queue and fence
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask	= 0; //single GPU env for now
	commandQueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Type		= COMMAND_LIST_TYPE;

	ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));

	// Initialize Fence event
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Create an event handle to use for frame synchronization.
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}


	// Initialization of Swap Chain
	// Get aplication window's handle (hwnd)
	auto windowHwnd = Win32Proc::GetHwnd();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount			= bufferCount;
	swapChainDesc.Width					= windowWidth;
	swapChainDesc.Height				= windowHeight;
	swapChainDesc.Format				= BACK_BUFFER_FORMAT;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.SampleDesc.Count		= 1;

	//auto swapChain1 = &dynamic_cast<IDXGISwapChain1*>(swapChain.Get());
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		windowHwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(windowHwnd, DXGI_MWA_NO_ALT_ENTER));

	// Convert SwapChain1 interface to SwapChain4
	ThrowIfFailed(swapChain1.As(&swapChain));
	currentFrameIdx = swapChain->GetCurrentBackBufferIndex();


	// Create RTV Descriptor Heaps
	constexpr auto descriptorHeapType	= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto descriptorHeapFlags	= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors	= bufferCount;
	rtvHeapDesc.Type			= descriptorHeapType;
	rtvHeapDesc.Flags			= descriptorHeapFlags;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	rtvIncrementDescriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

	// Get handle for 0th descriptor in heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (UINT n = 0; n < bufferCount; n++)
	{
		// Get pointer to swapChain buffer
		ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));

		// Create RenderTargetView 
		device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);

		// Offsets the rtv handle by descriptor size -> new_rtvHandle = rtvHandle + 1 * rtvDescriptorSize
		rtvHandle.Offset(1, rtvIncrementDescriptorSize);
	}


	// Initialize graphic pipelines
	// Create an empty root signature.
	{
		//CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		//rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		//ComPtr<ID3DBlob> signature;
		//ComPtr<ID3DBlob> error;
		//ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		//ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));



		//D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		//// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		//featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		//if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		//{
		//	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		//}

		//CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		//CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		//rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		//// Allow input layout and deny uneccessary access to certain pipeline stages.
		//D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		//	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		//	D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		//	D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		//	D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		//CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		//rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		//ComPtr<ID3DBlob> signature;
		//ComPtr<ID3DBlob> error;
		//ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		//ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));


		// create a root descriptor, which explains where to find the data for this root parameter
		D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
		rootCBVDescriptor.RegisterSpace			= 0;
		rootCBVDescriptor.ShaderRegister		= 0;

		// create a root parameter and fill it out
		D3D12_ROOT_PARAMETER rootParameters[1];										// only one parameter right now
		rootParameters[0].ParameterType			= D3D12_ROOT_PARAMETER_TYPE_CBV;	// this is a constant buffer view root descriptor
		rootParameters[0].Descriptor			= rootCBVDescriptor;				// this is the root descriptor for this root parameter
		rootParameters[0].ShaderVisibility		= D3D12_SHADER_VISIBILITY_VERTEX;	// our pixel shader will be the only shader accessing this parameter for now

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
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
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

		// Shaders Macro
		//constexpr D3D_SHADER_MACRO shaderDefines[] =
		//{
		//	"PIXEL_SHADER_ENABLED", "0",
		//	NULL, NULL
		//};

		constexpr DxcDefine shaderDefines[] =
		{
			L"PIXEL_SHADER_ENABLED", L"0",
			NULL, NULL
		};

		//constexpr const wchar_t* ShaderPath = L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//vertex_pixel_shader.hlsl";
		constexpr const wchar_t* vertexShaderPath	= L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//vertex_shader.hlsl";
		constexpr const wchar_t* pixelShaderPath	= L"E://!!PROJECTS_VS//DirectX//DXEngine//DXEngine//shaders//pixel_shader.hlsl";


		D3D12ShaderCompiler shaderCompiler;
		auto vertexShaderBlob = shaderCompiler.CompileShader(vertexShaderPath, nullptr, L"VSMain", L"vs_6_0");
		auto pixelShaderBlob = shaderCompiler.CompileShader(pixelShaderPath, shaderDefines, L"PSMain", L"ps_6_0");

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout					  = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature					= rootSignature.Get();
		psoDesc.VS								= CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		psoDesc.PS								= CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		psoDesc.RasterizerState					= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState						= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		//psoDesc.DepthStencilState.DepthEnable	= FALSE;
		//psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState				= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat						= DEPTH_STENCIL_FORMAT;
		psoDesc.SampleMask						= UINT_MAX;
		psoDesc.PrimitiveTopologyType			= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets				= 1;
		psoDesc.RTVFormats[0]					= BACK_BUFFER_FORMAT;
		psoDesc.SampleDesc.Count				= 1;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}


	// Initialize and create command list with pipeline state object and command allocator

	//Create the command allocator (as many as frame buffers)
	ThrowIfFailed(device->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&commandAllocator)));

	// Create the command list. (it is already in closed state)
	commandList = Utils::CreateGraphicsCommandList(device.Get(), 
												   CommandListDesc{0, COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAG_NONE},
												   commandAllocator.Get(),
												   pipelineState.Get());


	// Create the vertex buffer.
	{
		//// Define the geometry for a triangle.
		//Vertex triangleVertices[] =
		//{
		//	// First Quad
		//	//{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
		//	//{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	//{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		//	//{ { 0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },

		//	//// Second Quad
		//	//{ { -0.75f, 0.75f, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	//{ { 0.25f, -0.25f, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	//{ { -0.75f, -0.25f, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	//{ { 0.25f, 0.75f, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f } }

		//	//Triangle #1
		//	{ /*Position*/ {-1.0f, -1.0f, 0.9f} , /*Color*/ {1.0f, 1.0f, 1.0f, 1.0f}},
		//	{ /*Position*/ {0.0f, 1.0f, 0.9f} , /*Color*/ {1.0f, 1.0f, 1.0f, 1.0f}},
		//	{ /*Position*/ {1.0f, -1.0f, 0.9f} , /*Color*/ {1.0f, 1.0f, 1.0f, 1.0f}},

		//	//Triangle #2
		//	//{ /*Position*/ {-0.8f, -0.8f, 0.3f} , /*Color*/ {1.0f, 1.0f, 1.0f, 1.0f}},
		//	//{ /*Position*/ {0.0f, 1.0f, 0.3f} , /*Color*/ {0.0f, 1.0f, 1.0f, 1.0f}},
		//	//{ /*Position*/ {0.8f, -0.8f, 0.3f} , /*Color*/ {0.0f, 1.0f, 1.0f, 1.0f}},
		//	//{ /*Position*/ {0.8f, -0.8f, 0.3f} , /*Color*/ {0.0f, 1.0f, 1.0f, 1.0f}}
		//	{ { -0.75f, 0.75f, 0.7f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		//	{ { 0.25f, -0.25f, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	{ { -0.75f, -0.25f, 0.7f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		//	{ { 0.25f, 0.75f, 0.7f }, { 0.0f, 0.0f, 0.0f, 1.0f } }


		//	//{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
		//	//{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	//{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		//	//{ { 0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
		//};
		//constexpr UINT vertexBufferSize = sizeof(triangleVertices);

		//for (UINT i = 0; i < 3; ++i)
		//{
		//	Vertex NewPos;
		//	NewPos.position.x = triangleVertices[i].position.x * 0.5f;
		//	NewPos.position.y = triangleVertices[i].position.y * 0.5f;
		//	NewPos.position.z = triangleVertices[i].position.z;
		//	triangleVertices[i].position = NewPos.position;
		//}

		Vertex CubeVertices[] =
		{
			// front face
			{{ -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
			{{  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }},
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
			{{  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }},

			// right side face
			{{  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
			{{  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }},
			{{  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
			{{  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }},

			// left side face
			{{ -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
			{{ -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }},
			{{ -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
			{{ -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }},

			// back face
			{{  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
			{{ -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }},
			{{  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
			{{ -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }},

			// top face
			{{ -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
			{{ 0.5f,  0.5f,   0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }},
			{{ 0.5f,  0.5f, -0.5f  }, { 0.0f, 0.0f, 1.0f, 1.0f }},
			{{ -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }},

			// bottom face
			{{  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }},
			{{ -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }},
			{{  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }},
			{{ -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }}
		};

		constexpr UINT vertexBufferSize = sizeof(CubeVertices);
		
		// Create vertex buffer
		const auto VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&HEAP_PROPERTY_DEFAULT,								// a default heap
			D3D12_HEAP_FLAG_NONE,								// no flags
			&VertexBufferDesc,									// resource description for a buffer
			D3D12_RESOURCE_STATE_COPY_DEST,						// we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
			nullptr,											// optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
			IID_PPV_ARGS(&vertexBuffer)));
		NAME_D3D12_OBJECT(vertexBuffer, "VertexBuffer");

		// create upload heap
		// upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
		// We will upload the vertex buffer using this heap to the default heap
		ComPtr<ID3D12Resource> vBufferUploadHeap;
		device->CreateCommittedResource(
			&HEAP_PROPERTY_UPLOAD,								// upload heap
			D3D12_HEAP_FLAG_NONE,								// no flags
			&VertexBufferDesc,									// resource description for a buffer
			D3D12_RESOURCE_STATE_GENERIC_READ,					// GPU will read from this buffer and copy its contents to the default heap
			nullptr,
			IID_PPV_ARGS(&vBufferUploadHeap));
		NAME_D3D12_OBJECT(vBufferUploadHeap, "Vertex Buffer Upload Resource Heap");

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData		= reinterpret_cast<UINT8*>(CubeVertices);
		vertexData.RowPitch		= vertexBufferSize;
		vertexData.SlicePitch	= vertexData.RowPitch;

		UpdateSubresources<1>(commandList.Get(), vertexBuffer.Get(), vBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
		// transition the vertex buffer data from copy destination state to vertex buffer state
		const auto VertexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &VertexCmdListBarrier);

		//// Initialize the vertex buffer view.
		//vertexBufferView.BufferLocation		= vertexBuffer->GetGPUVirtualAddress();
		//vertexBufferView.StrideInBytes		= sizeof(Vertex);
		//vertexBufferView.SizeInBytes		= vertexBufferSize;

	}

	// Index Buffer
	{
		//// Indices
		//const DWORD indicesList[] = 
		//{
		//	0, 1, 2, // first triangle
		//	0, 3, 1, // second triangle
		//};

		//constexpr UINT indexBufferSize = sizeof(indicesList);

		//const auto uploadHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		//const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		//ThrowIfFailed(device->CreateCommittedResource(
		//	&uploadHeapDesc,
		//	D3D12_HEAP_FLAG_NONE,
		//	&indexBufferDesc,
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(&indexBuffer)));

		//// Copy the index data to the index buffer.
		//UINT8* pIndexDataBegin;
		//CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		//ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		//memcpy(pIndexDataBegin, indicesList, sizeof(indicesList));
		//indexBuffer->Unmap(0, nullptr);

		//// Initialize the index buffer view.
		//indexBufferView.BufferLocation	= indexBuffer->GetGPUVirtualAddress();
		//indexBufferView.Format			= DXGI_FORMAT_R32_UINT;
		//indexBufferView.SizeInBytes		= indexBufferSize;

		// Create index buffer

		// a quad (2 triangles)
		DWORD CubeIndices[] = {
			// ffront face
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

		constexpr UINT indexBufferSize = sizeof(CubeIndices);

		// create default heap to hold index buffer
		const auto IndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		device->CreateCommittedResource(
			&HEAP_PROPERTY_DEFAULT,								// a default heap
			D3D12_HEAP_FLAG_NONE,								// no flags
			&IndexBufferDesc,									// resource description for a buffer
			D3D12_RESOURCE_STATE_COPY_DEST,						// start in the copy destination state
			nullptr,											// optimized clear value must be null for this type of resource
			IID_PPV_ARGS(&indexBuffer));

		// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
		NAME_D3D12_OBJECT(indexBuffer, "Index Buffer Resource Heap");


		// create upload heap to upload index buffer
		ComPtr<ID3D12Resource> iBufferUploadHeap;
		device->CreateCommittedResource(
			&HEAP_PROPERTY_UPLOAD,								// upload heap
			D3D12_HEAP_FLAG_NONE,								// no flags
			&IndexBufferDesc,									// resource description for a buffer
			D3D12_RESOURCE_STATE_GENERIC_READ,					// GPU will read from this buffer and copy its contents to the default heap
			nullptr,
			IID_PPV_ARGS(&iBufferUploadHeap));
		NAME_D3D12_OBJECT(iBufferUploadHeap, "Index Buffer Upload Resource Heap");

		// store vertex buffer in upload heap
		D3D12_SUBRESOURCE_DATA indexData	= {};
		indexData.pData						= reinterpret_cast<BYTE*>(CubeIndices); // pointer to our index array
		indexData.RowPitch					= indexBufferSize; // size of all our index buffer
		indexData.SlicePitch				= indexBufferSize; // also the size of our index buffer

		// we are now creating a command with the command list to copy the data from
		// the upload heap to the default heap
		UpdateSubresources<1>(commandList.Get(), indexBuffer.Get(), iBufferUploadHeap.Get(), 0, 0, 1, &indexData);

		// transition the vertex buffer data from copy destination state to vertex buffer state
		const auto IndexBufferCmdBarrier = CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &IndexBufferCmdBarrier);
	}

	//Depth Stencil
	{
		// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors	= 1;
		dsvHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

		// create a depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format			= DEPTH_STENCIL_FORMAT;
		depthStencilViewDesc.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags			= D3D12_DSV_FLAG_NONE;

		// clear value for depth stencil
		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format						= DEPTH_STENCIL_FORMAT;
		depthOptimizedClearValue.DepthStencil.Depth			= 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil		= 0;

		// Create Depth Stencil Texture 2D
		const auto defaultHeapDesc		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const auto depthStencilDesc		= CD3DX12_RESOURCE_DESC::Tex2D(DEPTH_STENCIL_FORMAT, windowWidth, windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapDesc,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&depthStencil)));

		// Create Depth Stencil View
		device->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

		dsvIncrementDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	//Create Constant Buffer Data
	{
		// Describe and create a constant buffer view (CBV) descriptor heap.
		// Flags indicate that this descriptor heap can be bound to the pipeline 
		// and that descriptors contained in it can be referenced by a root table.
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors	= 1;
		cbvHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;


		// create the constant buffer resource heap
		// We will update the constant buffer one or more times per frame, so we will use only an upload heap
		// unlike previously we used an upload heap to upload the vertex and index data, and then copied over
		// to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
		// efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
		// will be modified and uploaded at least once per frame, so we only use an upload heap

		// first we will create a resource heap (upload heap) for each frame for the cubes constant buffers
		// As you can see, we are allocating 64KB for each resource we create. Buffer resource heaps must be
		// an alignment of 64KB. We are creating 3 resources, one for each frame. Each constant buffer is 
		// only a 4x4 matrix of floats in this tutorial. So with a float being 4 bytes, we have 
		// 16 floats in one constant buffer, and we will store 2 constant buffers in each
		// heap, one for each cube, thats only 64x2 bits, or 128 bits we are using for each
		// resource, and each resource must be at least 64KB (65536 bits)
		int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;
		const auto UploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
		for (int i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			// create resource for cube 1
			ThrowIfFailed(device->CreateCommittedResource(
				&HEAP_PROPERTY_UPLOAD,									// this heap will be used to upload the constant buffer data
				D3D12_HEAP_FLAG_NONE,									// no flags
				&UploadBufferDesc,										// size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
				D3D12_RESOURCE_STATE_GENERIC_READ,						// will be data that is read from so we keep it in the generic read state
				nullptr,												// we do not have use an optimized clear value for constant buffers
				IID_PPV_ARGS(&constantBufferUploadHeaps[i])));

			NAME_D3D12_OBJECT(constantBufferUploadHeaps[i], "Constant Buffer Upload Resource Heap");
			ZeroMemory(&cbvPerCube, ConstantBufferPerObjectSize);

			CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)

			// map the resource heap to get a gpu virtual address to the beginning of the heap
			ThrowIfFailed(constantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress[i])));

			// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
			// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
			memcpy(reinterpret_cast<void*>(cbvGPUAddress[i]), &cbvPerCube, ConstantBufferPerObjectSize);										// cube1's constant buffer data
			memcpy(reinterpret_cast<void*>(cbvGPUAddress[i] + ConstantBufferPerObjectAlignedSize), &cbvPerCube, ConstantBufferPerObjectSize);	// cube2's constant buffer data
		}

	}


	// Create View, Projection Matrices
	{
		// build projection and view matrix
		DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
		XMStoreFloat4x4(&camMatrices.cameraProjMat, tmpMat);

		// set starting camera state
		camMatrices.cameraPosition = DirectX::XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
		camMatrices.cameraTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		camMatrices.cameraUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

		// build view matrix
		DirectX::XMVECTOR cPos = DirectX::XMLoadFloat4(&camMatrices.cameraPosition);
		DirectX::XMVECTOR cTarg = DirectX::XMLoadFloat4(&camMatrices.cameraTarget);
		DirectX::XMVECTOR cUp = DirectX::XMLoadFloat4(&camMatrices.cameraUp);
		tmpMat = DirectX::XMMatrixLookAtLH(cPos, cTarg, cUp);
		XMStoreFloat4x4(&camMatrices.cameraViewMat, tmpMat);

		// set starting cubes position
		// first cube
		cube1Matrices.cubePosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
		DirectX::XMVECTOR posVec = DirectX::XMLoadFloat4(&cube1Matrices.cubePosition); // create xmvector for cube1's position

		tmpMat = DirectX::XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
		DirectX::XMStoreFloat4x4(&cube1Matrices.cubeRotMat, DirectX::XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
		DirectX::XMStoreFloat4x4(&cube1Matrices.cubeWorldMat, tmpMat); // store cube1's world matrix

		// second cube
		cube2Matrices.cubePosition = DirectX::XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
		//posVec = DirectX::XMLoadFloat4(&cube2Matrices.cube2PositionOffset) + DirectX::XMLoadFloat4(&cube1Matrices.cube1Position); // create xmvector for cube2's position
																					// we are rotating around cube1 here, so add cube2's position to cube1

		tmpMat = DirectX::XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
		DirectX::XMStoreFloat4x4(&cube2Matrices.cubeRotMat, DirectX::XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
		DirectX::XMStoreFloat4x4(&cube2Matrices.cubeWorldMat, tmpMat); // store cube2's world matrix
	}
}

void D3D12App::Render()
{
	// Get Current Frame index 
	currentFrameIdx = swapChain->GetCurrentBackBufferIndex();

	// Record some commands
	PopulateCommandLists();

	// Execute command lists
	DXCommandList* commandLists[] = { commandList.Get()};
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// Present the frame
	ThrowIfFailed(swapChain->Present(1, 0));

	//Wait for frame execution
	WaitForPreviousFrame();
}

void D3D12App::Update()
{

}

void D3D12App::Destroy()
{
	// Unmap mapped resource
	//constantBuffer->Unmap(0, nullptr);

	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	// Close handle of fence event
	CloseHandle(fenceEvent);
}

void D3D12App::PopulateCommandLists()
{
	// Reset previously used command list and command allocator
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	// Set viewport and scissors
	commandList->RSSetViewports(1, &viewPort);
	commandList->RSSetScissorRects(1, &scissorRect);


	// Set necessary state.
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	// set the root descriptor table 0 to the constant buffer descriptor heap

	// Record commands.

	// Render Target part

	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER barrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrierPresentToRTV);

	// Output Merger Set Render Target
	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrameIdx, rtvIncrementDescriptorSize);
	// get a handle to the depth/stencil buffer
	const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// Clear RT and Depth Stencil
	constexpr float clearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


	// Draw something
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
	//commandList->DrawInstanced(4, 1, 3, 0);

	//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//commandList->DrawIndexedInstanced(3, 1, 0, 0, 0); // draw first triangle
	commandList->DrawIndexedInstanced(6, 1, 0, 3, 0); // draw first quad






	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER barrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrierRTVtoPresent);

	// Close command list
	ThrowIfFailed(commandList->Close());
}

void D3D12App::WaitForPreviousFrame()
{
	// Signal and increment the fence value.
	fenceValue++;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

	// Wait until recorded commands are processed by GPU.
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}
