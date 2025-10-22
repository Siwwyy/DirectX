
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_HELPERS_H_INCLUDED
#define D3D12_HELPERS_H_INCLUDED

#include "pch.h"
#include <iostream>

/***************************
 ********* DEFINES *********
 ***************************/

 // Indicates if debug mode is set
#define DEBUG_MODE _DEBUG || DEBUG
//#define DEBUG_MODE 0

// Safe Release of ComPtr<T>
#define SAFE_RELEASE(p) if (p) (p)->Release()


// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x, NAME) x->SetName(L#NAME)
#define NAME_D3D12_OBJECT_INDEXED(x, n, NAME) SetNameIndexed((x)[n].Get(), L#NAME, n)


// Disables copy ability of specified class
#define DISABLE_COPY(CLASS_NAME)                            \
		CLASS_NAME(const CLASS_NAME&) = delete;             \
		CLASS_NAME& operator=(const CLASS_NAME&) = delete;  \

// Disables move ability of specified class
#define DISABLE_MOVE(CLASS_NAME)                            \
		CLASS_NAME(CLASS_NAME&&) = delete;                  \
		CLASS_NAME& operator=(CLASS_NAME&&) = delete;       \

// Disables both copy and move of specified class
#define DISABLE_COPY_MOVE(CLASS_NAME) DISABLE_COPY(CLASS_NAME) DISABLE_MOVE(CLASS_NAME) 


// assert with splitted condition and message inside
#define DXASSERT(CONDITION, MESSAGE) assert((CONDITION) && (MESSAGE))

// logging to console
#define DXLOG(MESSAGE, ...)                                  \
        char MsgBuffer[512]{};                               \
        sprintf_s(MsgBuffer, MESSAGE, __VA_ARGS__);          \
        OutputDebugStringA(MsgBuffer);                       \



#define DXFactory                       IDXGIFactory
#define DXFactory1                      IDXGIFactory1
#define DXFactory2                      IDXGIFactory2
#define DXFactory3                      IDXGIFactory3
#define DXFactory4                      IDXGIFactory4
#define DXDevice                        ID3D12Device
#define DXDevice1                       ID3D12Device1
#define DXDevice2                       ID3D12Device2
#define DXDevice3                       ID3D12Device3
#define DXDevice4                       ID3D12Device4
#define DXFence                         ID3D12Fence
#define DXResource                      ID3D12Resource
#define DXCommandQueue                  ID3D12CommandQueue
#define DXCommandList                   ID3D12CommandList
#define DXCommandList1                  ID3D12CommandList1
#define DXCommandList2                  ID3D12CommandList2
#define DXGraphicsCommandList           ID3D12GraphicsCommandList
#define DXGraphicsCommandList1          ID3D12GraphicsCommandList1
#define DXGraphicsCommandList2          ID3D12GraphicsCommandList2
#define DXGraphicsCommandList3          ID3D12GraphicsCommandList3
#define DXGraphicsCommandList4          ID3D12GraphicsCommandList4
#define DXGraphicsCommandList5          ID3D12GraphicsCommandList5
#define DXGraphicsCommandList6          ID3D12GraphicsCommandList6
#define DXCommandAllocator              ID3D12CommandAllocator
#define DXPipelineState                 ID3D12PipelineState
#define DXDescriptorHeap                ID3D12DescriptorHeap

#define DXDebugDevice                   ID3D12DebugDevice
#define DXDebugCommandQueue             ID3D12DebugCommandQueue
#define DXDebugCommandList              ID3D12DebugCommandList
#define DXDebugCommandList1             ID3D12DebugCommandList1
#define DXDebugCommandList2             ID3D12DebugCommandList2



/***************************
 ********* USINGS **********
 ***************************/

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace Helpers
{
    /***************************
	 ****** HELPER STRUCTS *****
	 ***************************/

    // Command List Helper
    struct CommandListDesc
    {
        UINT                        NodeMask;       //GPU Id
        D3D12_COMMAND_LIST_TYPE     Type;           //Command List type
        D3D12_COMMAND_LIST_FLAGS    Flags;          //Additional flags for Command List
    };


    // Root Param Helper
    struct RootParamHelper
    {
        D3D12_ROOT_PARAMETER_TYPE       ParameterType;
        D3D12_SHADER_VISIBILITY         ShaderVisibility;

        RootParamHelper(D3D12_ROOT_PARAMETER_TYPE ParameterType,
            D3D12_SHADER_VISIBILITY ShaderVisibility,
            D3D12_ROOT_CONSTANTS RootConstant)
            : ParameterType(ParameterType)
            , ShaderVisibility(ShaderVisibility)
        {
            ActiveField.Constants = RootConstant;
        }

        RootParamHelper(D3D12_ROOT_PARAMETER_TYPE ParameterType,
            D3D12_SHADER_VISIBILITY ShaderVisibility,
            D3D12_ROOT_DESCRIPTOR Descriptor)
            : ParameterType(ParameterType)
            , ShaderVisibility(ShaderVisibility)
        {
            ActiveField.Descriptor = Descriptor;
        }

        RootParamHelper(D3D12_ROOT_PARAMETER_TYPE ParameterType,
            D3D12_SHADER_VISIBILITY ShaderVisibility,
            D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable)
            : ParameterType(ParameterType)
            , ShaderVisibility(ShaderVisibility)
        {
            ActiveField.DescriptorTable = DescriptorTable;
        }

        D3D12_ROOT_PARAMETER CreateRootParameter() const noexcept
        {
            D3D12_ROOT_PARAMETER temp       = {};
            temp.ParameterType              = ParameterType;
            temp.ShaderVisibility           = ShaderVisibility;
            temp.Constants                  = ActiveField.Constants;
            temp.Descriptor                 = ActiveField.Descriptor;
            temp.DescriptorTable            = ActiveField.DescriptorTable;
            return temp;
        }

    private:
        struct /* NoName */
        {
            D3D12_ROOT_CONSTANTS        Constants;
            D3D12_ROOT_DESCRIPTOR       Descriptor;
            D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
        } ActiveField;

    };

    // Timer
    struct Timer
    {
        double timerFrequency = 0.0;
        long long lastFrameTime = 0;
        long long lastSecond = 0;
        double frameDelta = 0;
        int fps = 0;

        Timer()
        {
            LARGE_INTEGER li;
            QueryPerformanceFrequency(&li);

            // seconds
            //timerFrequency = double(li.QuadPart);

            // milliseconds
            timerFrequency = double(li.QuadPart) / 1000.0;

            // microseconds
            //timerFrequency = double(li.QuadPart) / 1000000.0;

            QueryPerformanceCounter(&li);
            lastFrameTime = li.QuadPart;
        }

        // Call this once per frame
        auto GetFrameDelta()
        {
            LARGE_INTEGER li;
            QueryPerformanceCounter(&li);
            frameDelta = double(li.QuadPart - lastFrameTime) / timerFrequency;
            if (frameDelta > 0)
            {
                fps = 1000 / frameDelta;
            }
                
            lastFrameTime = li.QuadPart;
            return frameDelta;
        }
    };

    /***************************
	 ******** FUNCTIONS ********
	 ***************************/
    ComPtr<IDXGIAdapter1>								                GetAdapter(ComPtr<IDXGIFactory1> pFactory, D3D_FEATURE_LEVEL deviceFeatureLevel, bool useWarpAdapter = false, bool requestHighPerformanceAdapter = false);


    // Inline definitions
    inline void ThrowIfFailed(HRESULT hr)
    {
        auto HrToString = [&]
        {
            char s_str[512] = {};
            sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
            /*OutputDebugStringA(static_cast<LPCSTR>(hr));*/
            return std::string{ s_str };
        };

        if (FAILED(hr))
        {
            //no need to pass argument to lambda
            throw std::runtime_error(HrToString());
        }
    }


    constexpr UINT CalculateConstantBufferByteSize(UINT byteSize) noexcept
    {
        // Constant buffer size is required to be aligned.
        return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
    }


    // Resets all elements in a ComPtr array.
    template<class T>
    constexpr void ResetComPtrArray(T* comPtrArray)
    {
        for (auto& i : *comPtrArray)
        {
            i.Reset();
        }
    }


    // Resets all elements in a unique_ptr array.
    template<class T>
    constexpr void ResetUniquePtrArray(T* uniquePtrArray)
    {
        for (auto& i : *uniquePtrArray)
        {
            i.reset();
        }
    }

    // make function for a ComPtr
    template <typename T, typename... Types, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    _NODISCARD constexpr ComPtr<T> make_com(Types&&... args)
	{ 
        return ComPtr<T>(new T(_STD forward<Types>(args)...));
    }

}

#endif //D3D12_HELPER_H_INCLUDED