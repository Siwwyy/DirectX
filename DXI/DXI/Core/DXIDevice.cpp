//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "DXIDevice.h"

// Namespaces
using namespace Helpers;

// Direct queue, everything this engine submits is graphics work
static constexpr D3D12_COMMAND_LIST_TYPE COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;

void DXIDevice::Init(const D3D_FEATURE_LEVEL FeatureLevel, const DXIDeviceOptions& Options)
{
	DXASSERT(!IsValid(), "Device has been already initialized");

	// The debug layer has to be enabled before the device exists, enabling it afterwards
	// invalidates the active device
	const UINT DxgiFactoryFlags = EnableDebugLayer(Options);

	//Create dxgi factory
	ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

	// Initialize device
	ComPtr<IDXGIAdapter1> Adapter;
	if (Options.bUseWarpAdapter)
	{
		// WARP is enumerated separately, it is not one of the hardware adapters
		ThrowIfFailed(Factory->EnumWarpAdapter(IID_PPV_ARGS(&Adapter)));
		DXLOG("Device: rendering on the WARP software adapter\n")
	}
	else
	{
		Adapter = GetAdapter(Factory, FeatureLevel);
	}

	ThrowIfFailed(D3D12CreateDevice(
		Adapter.Get(),
		FeatureLevel,
		IID_PPV_ARGS(&Device)
	));
	NAME_D3D12_OBJECT(Device, L"Main Device");

	SetBreakOnSeverity(Options.bBreakOnSeverity);

	// Initialization of command queue
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc	= {};
	CommandQueueDesc.Flags						= D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask					= 0; //single GPU env for now
	CommandQueueDesc.Priority					= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type						= COMMAND_LIST_TYPE;
	CommandQueue								= CreateCommandQueue(Device.Get(), CommandQueueDesc);
	NAME_D3D12_OBJECT(CommandQueue, L"Main Command Queue");
}

UINT DXIDevice::EnableDebugLayer(const DXIDeviceOptions& Options)
{
	UINT DxgiFactoryFlags = 0;

#if DEBUG_MODE
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	if (Options.bEnableDebugLayer)
	{
		ComPtr<ID3D12Debug3> DebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
		{
			// Cheap, it validates API usage on the CPU side
			DebugController->EnableDebugLayer();

			// Expensive, it instruments every shader, so it is opt in
			DebugController->SetEnableGPUBasedValidation(Options.bEnableGpuValidation);

			if (Options.bEnableGpuValidation)
			{
				DXLOG("Device: GPU based validation is on, expect it to be far slower\n")
			}

			// Enable additional debug layers.
			DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
		else
		{
			// Asking for it and not getting it is worth knowing about
			DXLOG("Device: the debug layer is unavailable, install the Graphics Tools optional feature\n")
		}
	}
	else
	{
		DXLOG("Device: debug layer disabled by -nodebuglayer\n")
	}

	//Check support for SSE2
	DXASSERT(XMVerifyCPUSupport(), "Verify Support of DirectXMath!");
#endif // DEBUG_MODE

	return DxgiFactoryFlags;
}

void DXIDevice::SetBreakOnSeverity(const bool bEnabled) const
{
#if DEBUG_MODE
	ComPtr<ID3D12InfoQueue> InfoQueue;
	if (FAILED(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue))))
	{
		// No info queue without the debug layer, which -nodebuglayer is free to turn off
		return;
	}

	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,	bEnabled);
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,		bEnabled);
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,			bEnabled);

	if (!bEnabled)
	{
		DXLOG("Device: not breaking on debug layer messages, they only go to the output\n")
	}
#endif // DEBUG_MODE
}

void DXIDevice::ReportLiveObjects() const
{
#if DEBUG_MODE
	if (!IsValid())
	{
		return;
	}

	// Stop breaking on the way out, the report itself goes through the info queue
	SetBreakOnSeverity(false);

	// Both of these only exist when the debug layer is on, which -nodebuglayer can turn
	// off, so a failure here is an absent interface rather than something to throw over
	ComPtr<ID3D12DebugDevice2> DebugDevice;
	if (FAILED(Device->QueryInterface(IID_PPV_ARGS(&DebugDevice))))
	{
		return;
	}

	ThrowIfFailed(DebugDevice->ReportLiveDeviceObjects(
		D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
	));
#endif // DEBUG_MODE
}
