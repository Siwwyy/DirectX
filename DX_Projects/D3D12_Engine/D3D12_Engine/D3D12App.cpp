#include "D3D12App.h"
#include "Win32Proc.h"

#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <dxcapi.h>

#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"
#include "ResourceBarrier.h"


//CONSTANTS
_CONSTEVAL D3D12_COMMAND_LIST_TYPE		COMMAND_LIST_TYPE		= D3D12_COMMAND_LIST_TYPE_DIRECT;
_CONSTEVAL UINT							BACK_BUFFER_COUNT		= 2;
_CONSTEVAL D3D_FEATURE_LEVEL			D3D12_FEATURE_LEVEL		= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
_CONSTEVAL DXGI_FORMAT					BACK_BUFFER_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
_CONSTEVAL DXGI_FORMAT					DEPTH_STENCIL_FORMAT	= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;		//depth stencil format

using namespace Helpers;
using namespace Math;


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
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");

	// Render Targets
	renderTargets.resize(BACK_BUFFER_COUNT);
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


	// Initialization of command queue
	commandQueue = std::make_unique<CommandQueue>(device.Get(), COMMAND_LIST_TYPE);


	// Initialization of Swap Chain
	// Get aplication window's handle (hwnd)
	auto windowHwnd = Win32Proc::GetHwnd();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Width = windowWidth;
	swapChainDesc.Height = windowHeight;
	swapChainDesc.Format = BACK_BUFFER_FORMAT;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.SampleDesc.Count = 1;

	//auto swapChain1 = &dynamic_cast<IDXGISwapChain1*>(swapChain.Get());
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		commandQueue->GetCommandQueue(),        // Swap chain needs the queue so that it can force a flush on it.
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
	constexpr auto descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto descriptorHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = descriptorHeapType;
	rtvHeapDesc.Flags = descriptorHeapFlags;
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
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif


		//constexpr const wchar_t* shaderPath = L"shaders/vertex_pixel_shader.hlsl";
		constexpr const wchar_t* shaderPath = L"E://!!PROJECTS_VS//DirectX//DX_Projects//D3D12_Engine//D3D12_Engine//shaders//vertex_pixel_shader.hlsl";
		//ThrowIfFailed(D3DCompileFromFile(shaderPath, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		//ThrowIfFailed(D3DCompileFromFile(shaderPath, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		D3D12ShaderCompiler shaderCompiler;
		auto vertexShaderBlob = shaderCompiler.CompileShaderD3D(shaderPath, nullptr, "VSMain", "vs_5_0");
		auto pixelShaderBlob = shaderCompiler.CompileShaderD3D(shaderPath, nullptr, "PSMain", "ps_5_0");

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();
		//psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		//psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = BACK_BUFFER_FORMAT;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}




	// Initialize and create command lists with pipeline state object
	commandList = std::make_unique<GraphicsCommandList1>(device.Get(), COMMAND_LIST_TYPE);




	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		const Vertex triangleVertices[] =
		{
			{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
			{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
			{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
		};

		constexpr UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		const auto uploadHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapDesc,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = vertexBufferSize;
	}
}

void D3D12App::Render()
{
	//Command list and allocator reset
	commandList->ResetCommandAllocator();
	commandList->ResetCommandList(pipelineState.Get());

	// Get Current Frame index 
	currentFrameIdx = swapChain->GetCurrentBackBufferIndex();

	// Record some commands
	PopulateCommandLists();

	// Close Command list before executing
	commandList->CloseCommandList();

	// Execute command lists
	DXCommandList* commandLists[] = { commandList->GetCommandList() };
	commandQueue->ExecuteCommandLists(commandLists);

	// Present the frame
	ThrowIfFailed(swapChain->Present(1, 0));

	// Wait for GPU to finish
	commandQueue->WaitForFence();
}

void D3D12App::Update()
{


}

void D3D12App::Destroy()
{

}

void D3D12App::PopulateCommandLists()
{
	const auto cmd = commandList->GetCommandList();

	// Set necessary state.
	cmd->SetGraphicsRootSignature(rootSignature.Get());
	cmd->RSSetViewports(1, &viewPort);
	cmd->RSSetScissorRects(1, &scissorRect);

	// Indicate that the back buffer will be used as a render target.
	//const CD3DX12_RESOURCE_BARRIER barrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	//commandList->ResourceBarrier(1, &barrierPresentToRTV);
	ResourceBarrier barrier(
		renderTargets[currentFrameIdx].Get(),
		cmd,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrameIdx, rtvIncrementDescriptorSize);
	cmd->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	constexpr float clearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	cmd->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	////commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmd->IASetVertexBuffers(0, 1, &vertexBufferView);
	cmd->DrawInstanced(4, 1, 0, 0);
}
