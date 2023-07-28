#include "D3D12App.h"
#include "D3D12Helper.h"
#include "Win32Proc.h"

//CONSTANTS
_CONSTEVAL D3D12_COMMAND_LIST_TYPE	COMMAND_LIST_TYPE			= D3D12_COMMAND_LIST_TYPE_DIRECT;
_CONSTEVAL UINT						BUFFER_COUNT				= 2;

D3D12App::D3D12App(UINT windowWidth, UINT windowHeight, std::wstring windowName)
	: windowWidth(windowWidth)
	, windowHeight(windowHeight)
	, aspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight))
	, windowName(windowName)
	, viewPort(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight))
	, scissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight))
	, bufferCount(BUFFER_COUNT)
	, currentFrameIdx(0)
	, rtvDescriptorSize(0)
{
	renderTargets.resize(bufferCount); //resize vector for handling only BUFFER_COUNT amount of render targets
}

void D3D12App::OnInit()
{
	InitializeFactory();
	InitializeDevice();
	InitializeCommandQueue();
	InitializeCommandAllocator();
	InitializeCommandList();
	InitializeSwapChain();
	InitializeFrameResources();
	InitializeFence();
}

void D3D12App::OnRender()
{
	// Populate Commands (just record command list and close)
	PopulateCommandLists();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3D12App::OnUpdate()
{

}

void D3D12App::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(fenceEvent);
}

void D3D12App::InitializeFactory()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
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
}

void D3D12App::InitializeDevice()
{
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	hardwareAdapter = GetAdapter(factory.Get());

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&device)
	));
}

void D3D12App::InitializeCommandQueue()
{
	// Create descriptor of Command Queue
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0; //Single GPU Environment
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Type = COMMAND_LIST_TYPE;

	ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));
}

void D3D12App::InitializeCommandAllocator()
{
	ThrowIfFailed(device->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&commandAllocator)));
	ThrowIfFailed(commandAllocator->Reset());
}

void D3D12App::InitializeCommandList()
{
	//TODO Add possibility for creating command list with PSO
	// Create the command list.
	ThrowIfFailed(device->CreateCommandList(0, COMMAND_LIST_TYPE, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(commandList->Close());
}

void D3D12App::InitializeSwapChain()
{
	// Get aplication window's handle (hwnd)
	auto windowHwnd = Win32Proc::GetHwnd();

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Width = windowWidth;
	swapChainDesc.Height = windowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
}

void D3D12App::InitializeFrameResources()
{
	// Create RTV Descriptor Heaps
	rtvHeap = CreateRenderTargetDescriptorHeap(device, rtvDescriptorSize);

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
}

void D3D12App::InitializeFence()
{
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;

	// Create an event handle to use for frame synchronization.
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

ComPtr<ID3D12DescriptorHeap> D3D12App::CreateRenderTargetDescriptorHeap(ComPtr<ID3D12Device> device, UINT& descriptorIncrementSize) const
{
	constexpr auto descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	constexpr auto descriptorHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = descriptorHeapType;
	rtvHeapDesc.Flags = descriptorHeapFlags;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);
	return descriptorHeap;
}

ComPtr<ID3D12Resource> D3D12App::CreateRenderTargetResource(ComPtr<ID3D12Device> device, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptorHandle)
{
	ComPtr<ID3D12Resource> renderTargetResource;
	device->CreateRenderTargetView(renderTargetResource.Get(), nullptr, renderTargetDescriptorHandle);
	return renderTargetResource;
}

void D3D12App::PopulateCommandLists()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

	// Set necessary state.
	//m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->RSSetViewports(1, &viewPort);
	commandList->RSSetScissorRects(1, &scissorRect);

	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER barrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrierPresentToRTV);

	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrameIdx, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	constexpr float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER barrierRTVtoPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[currentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrierRTVtoPresent);

	// Close Command list before executing
	ThrowIfFailed(commandList->Close());
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

ComPtr<IDXGIAdapter1> D3D12App::GetAdapter(IDXGIFactory1* pFactory, bool useWarpAdapter,
                                           bool requestHighPerformanceAdapter) const
{
	//If user specifies to use warp adapter, lets return it immediately
	if (useWarpAdapter)
	{
		ComPtr<IDXGIAdapter1> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		return warpAdapter;
	}

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory6;
	//Look for gpu based on requestHighPerformanceAdapter preferences
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, please specify useWarpAdapter to true
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				//it might raise exceptions! but it is ok, as nullptr is provided to D3D12CreateDevice!
				break;
			}
		}
	}

	//if we can not find adapter by its performance, look for first correct device
	if (!adapter.Get())
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	return adapter;
}
