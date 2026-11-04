#include "RenderGraph.h"
#include <pix3.h>

// Usings
using Helpers::ThrowIfFailed;

//// Ctors and Dctor
//RenderGraph::~RenderGraph()
//{
//	// Clean resources
//
//	// Close handle
//	CloseHandle(FenceEvent);
//
//#if DEBUG_MODE
//	{
//		{
//			ComPtr<ID3D12InfoQueue> InfoQueue;
//			ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
//			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
//			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
//			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
//		}
//
//		ComPtr<ID3D12DebugDevice2> DebugDevice;
//		ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&DebugDevice)));
//		Device->Release();
//		ThrowIfFailed(DebugDevice->ReportLiveDeviceObjects(
//			D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
//		));
//	}
//#endif // DEBUG_MODE
//}

RenderGraph::RenderGraph(ComPtr<DXDevice>& Device, ComPtr<DXGraphicsCommandList>& CommandList)
	: Device(Device)
	, CommandList(CommandList)
	, CurrentFrameIdx(0)
{
	DXASSERT(Device, "Device can not be null for RenderGraph construction");

	// If device is not null, initialize rest of resources needed to contruct the graph and execute passes
	//InitializeFrameResources();
}

void RenderGraph::InitializeFrameResources()
{
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
	}
}

void RenderGraph::WaitForPreviousFrame()
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

D3D12_VERTEX_BUFFER_VIEW RenderGraph::CreateVertexBufferView(const D3D12_GPU_VIRTUAL_ADDRESS VertexBufferGPUVirtualAddress, const UINT StrideInBytes, const UINT SizeInBytes)
{
	// create a index buffer view for the vertices. We get the GPU memory address to the index pointer using the GetGPUVirtualAddress() method later.
	D3D12_VERTEX_BUFFER_VIEW View;
	ZeroMemory(&View, sizeof(View));
	View.BufferLocation	= VertexBufferGPUVirtualAddress;
	View.StrideInBytes	= StrideInBytes; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	View.SizeInBytes	= SizeInBytes;
	return View;
}

D3D12_INDEX_BUFFER_VIEW RenderGraph::CreateIndexBufferView(const D3D12_GPU_VIRTUAL_ADDRESS IndexBufferGPUVirtualAddress, const DXGI_FORMAT IndexFormat, const UINT SizeInBytes)
{
	// create a index buffer view for the vertices. We get the GPU memory address to the index pointer using the GetGPUVirtualAddress() method later.
	D3D12_INDEX_BUFFER_VIEW View;
	ZeroMemory(&View, sizeof(View));
	View.BufferLocation = IndexBufferGPUVirtualAddress;
	View.Format			= IndexFormat; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	View.SizeInBytes	= SizeInBytes;
	return View;
}

void RenderGraph::TransitionBarrier(DXGraphicsCommandList * CommandList, BufferRef Buffer, D3D12_RESOURCE_STATES StateAfter)
{
	// transition the vertex buffer data from copy destination state to vertex buffer state
	const auto CmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(Buffer->Resource.Get(), Buffer->Desc.State, StateAfter);
	CommandList->ResourceBarrier(1, &CmdListBarrier);

	// Assign new state for resource after barrier
	Buffer->Desc.State = StateAfter;
}

void RenderGraph::Execute()
{
	for (std::size_t i = 0; i < Buffers.size(); ++i)
	{
		auto Resource	= Buffers[i];
		Resource->InitializeUnderlayingResource(Device.Get());
	}

	//// Begin Frame
	//BeginFrame();

	// Execute Passes
	for (std::size_t i = 0; i < Passes.size(); ++i)
	{
		auto Lambda = Passes[i];
		PIXBeginEvent(CommandList.Get(), 0, Lambda->JobName.c_str());
		Lambda->Execute(Device.Get(), CommandList.Get());
		PIXEndEvent(CommandList.Get());
	}

	//// End Frame
	//EndFrame();

	// Submit everything to GPU after collecting resources etc.
	//WaitForPreviousFrame();

	// Delete all the resources
	Buffers.clear();
}


void RenderGraph::BeginFrame()
{
	// Reset previously used command list and command allocator
	ThrowIfFailed(CommandAllocators[CurrentFrameIdx]->Reset());
	//ThrowIfFailed(CommandList->Reset(CommandAllocators[CurrentFrameIdx].Get(), PipelineState.Get()));

	// Get Current Frame index 
	CurrentFrameIdx = SwapChain->GetCurrentBackBufferIndex();

	// Indicate that the back buffer will be used as a render target.
	const CD3DX12_RESOURCE_BARRIER BarrierPresentToRTV = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &BarrierPresentToRTV);

	//// Set necessary state.
	//CommandList->SetGraphicsRootSignature(RootSignature.Get());
	//CommandList->RSSetViewports(1, &ViewPort);
	//CommandList->RSSetScissorRects(1, &ScissorRect);
	//CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//// get a handle to the depth/stencil buffer
	//CD3DX12_CPU_DESCRIPTOR_HANDLE DsvHandle(DsvHeap->GetCPUDescriptorHandleForHeapStart());
	//CommandList->ClearDepthStencilView(DsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// SV TARGET
	constexpr float ClearColor[] = { 0.0f, 0.5f, 1.0f, 1.0f };
	const CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentFrameIdx, RtvIncrementDescriptorSize);
	//CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, &DsvHandle);
	CommandList->ClearRenderTargetView(RtvHandle, ClearColor, 0, nullptr);
}

void RenderGraph::EndFrame()
{
	// Indicate that the back buffer will now be used to present.
	const CD3DX12_RESOURCE_BARRIER BarrierRTVToPresent = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[CurrentFrameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &BarrierRTVToPresent);
	// Close the command list and submit it to the GPU.
	ThrowIfFailed(CommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}