//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_DEVICE_H_INCLUDED
#define D3D12_DEVICE_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"

// How the device is brought up. Every one of these is a debugging knob, so they are driven
// off the command line rather than recompiled, see Utility/CommandLine.h.
struct D3D12DeviceOptions
{
	// Instruments every shader to catch out of bounds access and uninitialised reads.
	// Commonly 10x slower or worse, so it stays off unless something needs chasing.
	bool bEnableGpuValidation	= false;

	// The CPU side validation. Cheap and worth leaving on, but it needs the Graphics Tools
	// optional feature installed, so there has to be a way to turn it off.
	bool bEnableDebugLayer		= true;

	// Whether a debug layer message halts in the debugger. Breaking on WARNING is
	// aggressive and some drivers are chatty enough to make it unusable.
	bool bBreakOnSeverity		= true;

	// The software rasterizer. Slow, but it renders the same frame without a vendor driver
	// in the way, which is how you tell a driver bug from an engine bug.
	bool bUseWarpAdapter		= false;
};

// Factory, adapter, device and the direct command queue.
// Every D3D12 app needs the exact same setup, so it lives here instead of being
// copied into each one of them.
class D3D12Device
{
	DISABLE_COPY_MOVE(D3D12Device)

public:

	D3D12Device()	= default;
	~D3D12Device()	= default;

	// Turns on the debug layer, creates the factory, picks an adapter and creates the
	// device along with its direct command queue.
	void Init(const D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
					const D3D12DeviceOptions& Options = {});

	// Reports whatever is still alive, call it once everything else has been released
	void ReportLiveObjects() const;

	// Getters
	_NODISCARD FORCEINLINE DXDevice* Get() const noexcept
	{
		return Device.Get();
	}
	_NODISCARD FORCEINLINE DXFactory4* GetFactory() const noexcept
	{
		return Factory.Get();
	}
	_NODISCARD FORCEINLINE DXCommandQueue* GetCommandQueue() const noexcept
	{
		return CommandQueue.Get();
	}

	// Some helpers still take the device as a ComPtr, e.g. GraphicsPSO::SetupPSO
	_NODISCARD FORCEINLINE const ComPtr<DXDevice>& GetComPtr() const noexcept
	{
		return Device;
	}
	_NODISCARD FORCEINLINE bool IsValid() const noexcept
	{
		return Device != nullptr;
	}

private:

	// Turns the debug layer on, has to happen before the device is created
	_NODISCARD static UINT EnableDebugLayer(const D3D12DeviceOptions& Options);

	// Whether a debug layer message halts in the debugger
	void SetBreakOnSeverity(const bool bEnabled) const;

	ComPtr<DXFactory4>		Factory;		// DXGI factory the adapter and swap chain come from
	ComPtr<DXDevice>		Device;			// the device itself
	ComPtr<DXCommandQueue>	CommandQueue;	// direct queue every pass is submitted to
};

#endif // D3D12_DEVICE_H_INCLUDED
