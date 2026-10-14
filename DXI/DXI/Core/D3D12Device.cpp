//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "D3D12Device.h"

// Namespaces
using namespace Helpers;

// Direct queue, everything this engine submits is graphics work
static constexpr D3D12_COMMAND_LIST_TYPE COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;

void D3D12Device::Initialize(const D3D_FEATURE_LEVEL FeatureLevel, const bool bEnableGpuValidation)
{
	DXASSERT(!IsValid(), "Device has been already initialized");

	// The debug layer has to be enabled before the device exists, enabling it afterwards
	// invalidates the active device
	const UINT DxgiFactoryFlags = EnableDebugLayer(bEnableGpuValidation);

	//Create dxgi factory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

	// Initialize device
	const ComPtr<IDXGIAdapter1> HardwareAdapter = GetAdapter(Factory, FeatureLevel);
	ThrowIfFailed(D3D12CreateDevice(
		HardwareAdapter.Get(),
		FeatureLevel,
		IID_PPV_ARGS(&Device)
	));
	NAME_D3D12_OBJECT(Device, L"Main Device");

	SilenceDebugBreaks();

	// Initialization of command queue
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc	= {};
	CommandQueueDesc.Flags						= D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask					= 0; //single GPU env for now
	CommandQueueDesc.Priority					= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type						= COMMAND_LIST_TYPE;
	CommandQueue								= CreateCommandQueue(Device.Get(), CommandQueueDesc);
	NAME_D3D12_OBJECT(CommandQueue, L"Main Command Queue");
}

UINT D3D12Device::EnableDebugLayer(const bool bEnableGpuValidation)
{
	UINT DxgiFactoryFlags = 0;

#if DEBUG_MODE
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	{
		ComPtr<ID3D12Debug3> DebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
		{
			// Cheap, it validates API usage on the CPU side, so it always stays on
			DebugController->EnableDebugLayer();

			// Expensive, it instruments every shader, so it is opt in
			DebugController->SetEnableGPUBasedValidation(bEnableGpuValidation);

			// Enable additional debug layers.
			DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}

	//Check support for SSE2
	DXASSERT(XMVerifyCPUSupport(), "Verify Support of DirectXMath!");
#endif // DEBUG_MODE

	return DxgiFactoryFlags;
}

void D3D12Device::SilenceDebugBreaks() const
{
#if DEBUG_MODE
	ComPtr<ID3D12InfoQueue> InfoQueue;
	ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,	true);
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,		true);
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,			true);
#endif // DEBUG_MODE
}

void D3D12Device::ReportLiveObjects() const
{
#if DEBUG_MODE
	if (!IsValid())
	{
		return;
	}

	// Stop breaking on the way out, the report itself goes through the info queue
	{
		ComPtr<ID3D12InfoQueue> InfoQueue;
		ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue)));
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,	false);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,		false);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,			false);
	}

	ComPtr<ID3D12DebugDevice2> DebugDevice;
	ThrowIfFailed(Device->QueryInterface(IID_PPV_ARGS(&DebugDevice)));
	ThrowIfFailed(DebugDevice->ReportLiveDeviceObjects(
		D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
	));
#endif // DEBUG_MODE
}
