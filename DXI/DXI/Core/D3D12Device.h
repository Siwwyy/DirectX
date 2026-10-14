//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_DEVICE_H_INCLUDED
#define D3D12_DEVICE_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"

// Factory, adapter, device and the direct command queue.
// Every D3D12 app needs the exact same setup, so it lives here instead of being
// copied into each one of them.
class D3D12Device
{
	DISABLE_COPY_MOVE(D3D12Device)

public:

	D3D12Device()	= default;
	~D3D12Device()	= default;

	// Turns on the debug layer, creates the factory, picks a hardware adapter
	// and creates the device along with its direct command queue.
	// GPU based validation instruments every shader and commonly costs 10x or more,
	// so it stays off unless something actually needs chasing down.
	void Initialize(const D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
					const bool bEnableGpuValidation = false);

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
	_NODISCARD static UINT EnableDebugLayer(const bool bEnableGpuValidation);

	// Stops the debug layer from breaking into the debugger on every message
	void SilenceDebugBreaks() const;

	ComPtr<DXFactory4>		Factory;		// DXGI factory the adapter and swap chain come from
	ComPtr<DXDevice>		Device;			// the device itself
	ComPtr<DXCommandQueue>	CommandQueue;	// direct queue every pass is submitted to
};

#endif // D3D12_DEVICE_H_INCLUDED
