#include "D3D12App.h"
#include "Win32Proc.h"

#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <dxcapi.h>

#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"


//CONSTANTS
_CONSTEVAL D3D12_COMMAND_LIST_TYPE		COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;
_CONSTEVAL UINT							BACK_BUFFER_COUNT = 2;
_CONSTEVAL D3D_FEATURE_LEVEL			D3D12_FEATURE_LEVEL = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
_CONSTEVAL DXGI_FORMAT					BACK_BUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

using namespace Helpers;
using namespace Math;

D3D12App::D3D12App(const UINT windowWidth, const UINT windowHeight, const std::wstring windowName)
	: windowWidth(windowWidth)
	, windowHeight(windowHeight)
	, aspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight))
	, windowName(windowName)
	, viewPort(0.f, 0.f, static_cast<float>(windowWidth), static_cast<float>(windowHeight))
	, scissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight))
	, bufferCount(BACK_BUFFER_COUNT)
	, currentFrameIdx(0)
	, rtvDescriptorSize(0)
{
	renderTargets.resize(BACK_BUFFER_COUNT);
}

void D3D12App::Initialize()
{
	// Initialize factory
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG) || defined(DEBUG)
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



	// Initialize Device
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	hardwareAdapter = GetAdapter(factory, D3D12_FEATURE_LEVEL);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&device)
	));



	// Initialize Command Queue
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0; //Single GPU Environment
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Type = COMMAND_LIST_TYPE;

	ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));



	// Initialize Command Allocator
	ThrowIfFailed(device->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&commandAllocator)));
	ThrowIfFailed(commandAllocator->Reset()); //reset it just in case, before recording any command



	// Initialize Back Buffers RT (RTV as well)
	constexpr auto DESCRIPTOR_TYPE_RTV = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0; //Single GPU Environment
	descriptorHeapDesc.NumDescriptors = bufferCount;
	descriptorHeapDesc.Type = DESCRIPTOR_TYPE_RTV;
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(DESCRIPTOR_TYPE_RTV); //get size of descriptor



	// Initialize Swap Chain
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



	// Initialize Frame Buffer (Back Buffers) Resources
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
		rtvHandle.Offset(1, rtvDescriptorSize);
	}



	// Initialize Fence
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;

	// Create an event handle to use for frame synchronization.
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}



	// Initialize Root Signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));



	// Compile shaders to blob (Using New API IDxc)

#if defined(_DEBUG) || defined(DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	// Shaders Macro
	const DxcDefine shaderDefines[] =
	{
		L"PIXEL_SHADER_ENABLED", L"1",
		NULL, NULL
	};

	constexpr LPCWSTR shaderPath = L"E://!!PROJECTS_VS//DirectX//DX_Projects//D3D12Application//D3D12Application//shaders//vertex_pixel_shader.hlsl";
	D3D12ShaderCompiler shaderCompiler;

	// compile Vertex Shader
	auto compiledVSBlob = shaderCompiler.CompileShader(
		shaderPath,
		shaderDefines,
		L"VSMain",
		L"vs_6_0");

	// Compile Pixel Shader
	auto compiledPSBlob = shaderCompiler.CompileShader(
		shaderPath,
		shaderDefines,
		L"PSMain",
		L"ps_6_0");



	// Initialize PSO
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	CD3DX12_SHADER_BYTECODE vsShaderByteCode(compiledVSBlob.Get());
	CD3DX12_SHADER_BYTECODE psShaderByteCode(compiledPSBlob.Get());
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = vsShaderByteCode;
	psoDesc.PS = psShaderByteCode;
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



	// Initialize Command List
	ThrowIfFailed(device->CreateCommandList(0, COMMAND_LIST_TYPE, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(commandList->Close());



	// Vertex Buffer stuffs
	// Create the vertex buffer.
	// Define the geometry for a triangle (in this case we will use it for making a rectangle, 4 vertices).
	const Vertex triangleVertices[] =
	{
		{ { -0.5f,  0.5f, 0.0f, }, { 1.0f, 0.0f, 1.0f, 1.0f } },
		{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
	};

	constexpr UINT triangleBufferSize			= sizeof(triangleVertices);

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	const auto uploadHeapDesc				= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto vertexBufferDesc				= CD3DX12_RESOURCE_DESC::Buffer(triangleBufferSize);
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
	memcpy(pVertexDataBegin, triangleVertices, triangleBufferSize);
	vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation			= vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes			= sizeof(Vertex);
	vertexBufferView.SizeInBytes			= triangleBufferSize;

	// Index buffer initialization and creation
	DWORD indicesList[] = {
		0, 1, 2, // first triangle
		0, 3, 1 // second triangle
	};

	constexpr UINT indicesListSize			= sizeof(indicesList);
	const auto indexBufferDesc				= CD3DX12_RESOURCE_DESC::Buffer(indicesListSize);

	// create upload heap to upload index buffer
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapDesc,
		D3D12_HEAP_FLAG_NONE,
		&indexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)));

	// Copy the index data to the index buffer.
	UINT8* pVertexDataBegin1;
	CD3DX12_RANGE readRange1(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(indexBuffer->Map(0, &readRange1, reinterpret_cast<void**>(&pVertexDataBegin1)));
	memcpy(pVertexDataBegin1, indicesList, indicesListSize);
	indexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	indexBufferView.BufferLocation	= indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format			= DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes		= indicesListSize;

	WaitForPreviousFrame();
}

void D3D12App::Render()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	// Set necessary states.
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->RSSetViewports(1, &viewPort);
	commandList->RSSetScissorRects(1, &scissorRect);

	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER barrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrierPresentToRTV);

	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrameIdx, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	constexpr float clearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// draw rectangle using one triangle with indices
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // set the vertex buffer (using the vertex buffer view)
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0); // draw 2 triangles (draw 1 instance of 2 triangles)
	////commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	//commandList->DrawInstanced(4, 1, 0, 0);





	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER barrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrierRTVtoPresent);

	// Close Command list before executing
	ThrowIfFailed(commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3D12App::Update()
{
	//// Command list allocators can only be reset when the associated 
	//// command lists have finished execution on the GPU; apps should use 
	//// fences to determine GPU execution progress.
	//ThrowIfFailed(commandAllocator->Reset());

	//// However, when ExecuteCommandList() is called on a particular command 
	//// list, that command list can then be reset at any time and must be before 
	//// re-recording.
	//ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	//// Update vertices
	//// Vertex Buffer stuffs
	//// Create the vertex buffer.
	//// Define the geometry for a triangle.
	//if (currentFrameIdx % 2)
	//{
	//	const Vertex triangleVertices[] =
	//	{
	//		{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
	//		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
	//		{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
	//		{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
	//	};

	//	constexpr UINT vertexBufferSize = sizeof(triangleVertices);

	//	// Copy the triangle data to the vertex buffer.
	//	UINT8* pVertexDataBegin;
	//	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	//	ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	//	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	//	vertexBuffer->Unmap(0, nullptr);

	//	// Initialize the vertex buffer view.
	//	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	//	vertexBufferView.StrideInBytes = sizeof(Vertex);
	//	vertexBufferView.SizeInBytes = vertexBufferSize;
	//}
	//else
	//{
	//	const Vertex triangleVertices[] =
	//	{
	//		{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
	//		{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
	//		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
	//		{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
	//	};


	//	constexpr UINT vertexBufferSize = sizeof(triangleVertices);

	//	// Copy the triangle data to the vertex buffer.
	//	UINT8* pVertexDataBegin;
	//	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	//	ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	//	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	//	vertexBuffer->Unmap(0, nullptr);

	//	// Initialize the vertex buffer view.
	//	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	//	vertexBufferView.StrideInBytes = sizeof(Vertex);
	//	vertexBufferView.SizeInBytes = vertexBufferSize;
	//}

	//// Close Command list before executing
	//ThrowIfFailed(commandList->Close());

	//// Execute the command list.
	//ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	//commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//WaitForPreviousFrame();
}

void D3D12App::Destroy()
{
	//// Release all the pointers
	//SAFE_RELEASE(factory);
	//SAFE_RELEASE(device);
	//SAFE_RELEASE(commandQueue);
	//SAFE_RELEASE(commandAllocator);
	//SAFE_RELEASE(commandList);
	//SAFE_RELEASE(pipelineState);
	//SAFE_RELEASE(rootSignature);
	//SAFE_RELEASE(vertexBuffer);
	//SAFE_RELEASE(swapChain);
	//SAFE_RELEASE(rtvHeap);
	//SAFE_RELEASE(fence);

	//// Release all the array of pointers
	//ResetComPtrArray(&renderTargets);
}

void D3D12App::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

	// Signal and increment the fence value.
	const UINT64 fenceCurrentValue = fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceCurrentValue));
	fenceValue++;

	// Wait until the previous frame is finished.
	if (fence->GetCompletedValue() < fenceCurrentValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceCurrentValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	currentFrameIdx = swapChain->GetCurrentBackBufferIndex();
}
