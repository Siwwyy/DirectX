
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Helpers.h"

#include <dxgi1_6.h>


ComPtr<IDXGIAdapter1> Helpers::GetAdapter(ComPtr<IDXGIFactory1> pFactory, D3D_FEATURE_LEVEL deviceFeatureLevel, bool useWarpAdapter,
	bool requestHighPerformanceAdapter)
{
	//If user specifies to use warp adapter, lets return it immediately
	if (useWarpAdapter)
	{
		ComPtr<IDXGIAdapter1> warpAdapter;
		ComPtr<IDXGIFactory4> tempFactory4;
		ThrowIfFailed(tempFactory4.As(&pFactory)); //check if conversion to other Interface is correct
		ThrowIfFailed(tempFactory4->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
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
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), deviceFeatureLevel, __uuidof(ID3D12Device), nullptr)))
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
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), deviceFeatureLevel, __uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	return adapter;
}