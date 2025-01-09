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
static ComPtr<ID3DBlob>						VertexShader				= nullptr;												// Vertex shader blob
static ComPtr<ID3DBlob>						PixelShader					= nullptr;												// Pixel shader blob




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
	//, camMatrices({})
	//, cube1Matrices({})
	//, cube2Matrices({})
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");

	//// Fences
	//Fences.resize(BACK_BUFFER_COUNT);
	//// Command allocators
	//CommandAllocator.resize(BACK_BUFFER_COUNT);
	//// Render Targets
	//RenderTargets.resize(BACK_BUFFER_COUNT);
	//// Constant buffer heaps
	//ConstantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	//// Constat buffer gpu addresses
	//CbvGPUAddress.resize(BACK_BUFFER_COUNT);
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


	// Initialization of Swap Chain
	// Get aplication window's handle (hwnd)
	auto windowHwnd = Win32Proc::GetHwnd();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
	swapChainDesc.Width = WindowWidth;
	swapChainDesc.Height = WindowHeight;
	swapChainDesc.Format = BACK_BUFFER_FORMAT;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.SwapEffect				= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.SampleDesc.Count = 1;

	auto swapChain1 = dynamic_cast<IDXGISwapChain1*>(SwapChain.Get()); //retrieve IDXGISwapChain1 from IDXGISwapChain3

	ThrowIfFailed(Factory->CreateSwapChainForHwnd(
		CommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		windowHwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(Factory->MakeWindowAssociation(windowHwnd, DXGI_MWA_NO_ALT_ENTER));
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Create RTV Descriptor Heaps
	constexpr auto descriptorHeapType		= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto descriptorHeapFlags		= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc	= {};
	rtvHeapDesc.NumDescriptors				= BACK_BUFFER_COUNT;
	rtvHeapDesc.Type						= descriptorHeapType;
	rtvHeapDesc.Flags						= descriptorHeapFlags;
	ThrowIfFailed(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	// Get Render Target View Increment Descriptor size
	RtvIncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(descriptorHeapType);


	// Initialization of command queue
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc;
	CommandQueueDesc.Flags			= D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask		= 0; //single GPU env for now
	CommandQueueDesc.Priority		= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type			= COMMAND_LIST_TYPE;
	CommandQueue = Utils::CreateCommandQueue(Device.Get(), CommandQueueDesc);

	// Create an event handle to use for frame synchronization.
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	//Initialize resources per frame buffer
	InitializePerFrameResources();

	//Initialization of command list
	CommandList = Utils::CreateGraphicsCommandList(
					Device.Get(),
					CommandListDesc{ 0, COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAG_NONE },
					CommandAllocators[0].Get(),
					PipelineState.Get());

	ThrowIfFailed(CommandList->Close()); //close not closed command list

	// Root parameters
	{
		// create a root descriptor, which explains where to find the data for this root parameter
		D3D12_ROOT_DESCRIPTOR rootCBVDescriptor		= {};
		rootCBVDescriptor.RegisterSpace				= 0;
		rootCBVDescriptor.ShaderRegister			= 0;

		// create a root parameter and fill it out
		D3D12_ROOT_PARAMETER rootParameters[1]		= {};										// only one parameter right now
		rootParameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;			// this is a constant buffer view root descriptor
		rootParameters[0].Descriptor				= rootCBVDescriptor;						// this is the root descriptor for this root parameter
		rootParameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;			// our pixel shader will be the only shader accessing this parameter for now

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
		ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

	}

	//Shaders compilation
	{


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
		auto vertexShaderBlob	= shaderCompiler.CompileShader(vertexShaderPath, nullptr, L"VSMain", L"vs_6_0");
		auto pixelShaderBlob	= shaderCompiler.CompileShader(vertexShaderPath, shaderDefines, L"PSMain", L"ps_6_0");
	}

}

void D3D12App::Render()
{
}

void D3D12App::Update()
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
	// Command allocators
	CommandAllocators.resize(BACK_BUFFER_COUNT);
	// Render Targets
	RenderTargets.resize(BACK_BUFFER_COUNT);
	// Constant buffer heaps
	//ConstantBufferUploadHeaps.resize(BACK_BUFFER_COUNT);
	//// Constat buffer gpu addresses
	//CbvGPUAddress.resize(BACK_BUFFER_COUNT);

	// Get handle for 0th descriptor in heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < BACK_BUFFER_COUNT; ++i)
	{
		//Create fences
		ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fences[i])));

		//Create the command allocators
		ThrowIfFailed(Device->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&CommandAllocators[i])));

		{
			// Get pointer to swapChain buffer
			ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTargets[i])));

			// Create RenderTargetView 
			Device->CreateRenderTargetView(RenderTargets[i].Get(), nullptr, rtvHandle);

			// Offsets the rtv handle by descriptor size -> new_rtvHandle = rtvHandle + 1 * rtvDescriptorSize
			rtvHandle.Offset(1, RtvIncrementDescriptorSize);
		}
	}
}

void D3D12App::SubmitCommandLists()
{
	// Reset previously used command list and command allocator
	ThrowIfFailed(CommandAllocators[CurrentFrameIdx]->Reset());
	ThrowIfFailed(CommandList->Reset(CommandAllocators[CurrentFrameIdx].Get(), PipelineState.Get()));

	// Execute command lists
	ThrowIfFailed(CommandList->Close()); //close not closed command list
	DXCommandList* commandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void D3D12App::PopulateCommandLists()
{
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


