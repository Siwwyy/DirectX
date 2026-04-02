#include "RenderGraph.h"
#include <pix3.h>

// Usings
using Helpers::ThrowIfFailed;

//// Ctors and Dctor
//RenderGraph::~RenderGraph()
//{
//	// Clean resources
//	for (std::size_t i = 0; i < Buffers.size(); ++i)
//	{
//		delete Buffers[i].first;
//	}
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
{
}

// Initializers
void RenderGraph::InitializeDevice()
{
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
#endif // DEBUG_MODE

	//Create dxgi factory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

	// Initialize device
	const ComPtr<IDXGIAdapter1> HardwareAdapter = Helpers::GetAdapter(Factory, D3D12_FEATURE_LEVEL);
	ThrowIfFailed(D3D12CreateDevice(
		HardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&Device)
	));

#if DEBUG_MODE
	{
		ComPtr<ID3D12InfoQueue> InfoQueue;
		ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif // DEBUG_MODE
}

void RenderGraph::InitializePerFrameResources()
{
	DXASSERT(Device, "Device can not be null for Initialization per frame resources");

	// Resize arrays/vectors per frame buffer resources
	// Fences
	Fences.resize(BACK_BUFFER_COUNT);
	// Fence Values
	FenceValue.resize(BACK_BUFFER_COUNT);
	// Command allocators
	CommandAllocators.resize(BACK_BUFFER_COUNT);

	// Loop
	for (UINT i = 0; i < BACK_BUFFER_COUNT; ++i)
	{
		//Create fences
		ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fences[i])));

		//Create the command allocators
		ThrowIfFailed(Device->CreateCommandAllocator(COMMAND_LIST_TYPE, IID_PPV_ARGS(&CommandAllocators[i])));
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

	for (std::size_t i = 0; i < Passes.size(); ++i)
	{
		auto Lambda = Passes[i];
		//PIXBeginEvent(CommandList.Get(), 0, Lambda->JobName.c_str());
		Lambda->Execute(Device.Get(), CommandList.Get());
		//PIXEndEvent(CommandList.Get());
	}

	// Submit everything to GPU after collecting resources etc.
	//WaitForPreviousFrame();

	// Delete all the resources
	Buffers.clear();
}
