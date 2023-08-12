
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once

#include <dxgi.h>
#include <wrl/client.h>
#include "d3dx12.h"

// Safe Release of ComPtr<T>
#define SAFE_RELEASE(p) if (p) (p)->Release()
// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

// Usings
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// Functions
namespace Helpers
{

    inline void                                                         ThrowIfFailed(HRESULT hr);
    constexpr UINT                                                      CalculateConstantBufferByteSize(UINT byteSize) noexcept;
    ComPtr<IDXGIAdapter1>								                GetAdapter(ComPtr<IDXGIFactory1> pFactory, D3D_FEATURE_LEVEL deviceFeatureLevel, bool useWarpAdapter = false, bool requestHighPerformanceAdapter = false);



    // Resets all elements in a ComPtr array.
    template<class T>
    void ResetComPtrArray(T* comPtrArray)
    {
        for (auto& i : *comPtrArray)
        {
            i.Reset();
        }
    }

    // Resets all elements in a unique_ptr array.
    template<class T>
    void ResetUniquePtrArray(T* uniquePtrArray)
    {
        for (auto& i : *uniquePtrArray)
        {
            i.reset();
        }
    }

}