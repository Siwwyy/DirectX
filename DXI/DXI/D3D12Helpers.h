
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

// Safe Release of ComPtr<T>
#define SAFE_RELEASE(p) if (p) (p)->Release()


// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x, NAME) x->SetName(NAME)
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

// logging to console | Curly brackets are for keeping MsgBuffer inside scope
#define DXLOG(MESSAGE, ...)                                 \
        {                                                   \
            char MsgBuffer[512]{};                          \
            sprintf_s(MsgBuffer, MESSAGE, __VA_ARGS__);     \
            OutputDebugStringA(MsgBuffer);                  \
        }                                                   \


// Pre-defined types
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



// HEAP PROPERTIES
const CD3DX12_HEAP_PROPERTIES   DX_HEAP_PROPERTY_DEFAULT    = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);		//Heap type default
const CD3DX12_HEAP_PROPERTIES	DX_HEAP_PROPERTY_UPLOAD     = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		//Heap type upload
const CD3DX12_HEAP_PROPERTIES	DX_HEAP_PROPERTY_READBACK   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);	//Heap type readback
const CD3DX12_HEAP_PROPERTIES	DX_HEAP_PROPERTY_CUSTOM     = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_CUSTOM);		//Heap type custom

// TRANSFORMS
#define DX_IDENTITY_SCALE       XMFLOAT3(1.f, 1.f, 1.f)
#define DX_IDENTITY_ROTATE      XMFLOAT3(0.f, 0.f, 0.f)
#define DX_IDENTITY_TRANSFORM   XMFLOAT3(0.f, 0.f, 0.f)


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

    // SAMPLER HELPER
    struct SAMPLER_HELPER
    {
        static DXGI_SAMPLE_DESC CreateSampler(UINT count, UINT quality)
        {
            DXGI_SAMPLE_DESC temp;
            temp.Count = count;
            temp.Quality = quality;
            return temp;
        }
    };

    // VERTEX HELPER
    struct VERTEX_HELPER
    {
        DISABLE_COPY(VERTEX_HELPER)
        
        // Ctor
        VERTEX_HELPER(DXDevice * Device, const UINT VertexBufferSize, 
            D3D12_HEAP_PROPERTIES HEAP_PROPERTY = DX_HEAP_PROPERTY_DEFAULT, 
            D3D12_RESOURCE_STATES RESOURCE_STATE = D3D12_RESOURCE_STATE_COMMON,
            LPCWSTR Name = L"VertexBuffer");

        // Create View for Vertex Buffer
        _NODISCARD D3D12_VERTEX_BUFFER_VIEW CreateView(const UINT StrideInBytes, const UINT SizeInBytes);

        // Get pointer
        _NODISCARD auto GetPointer()
        {
            if (!VertexBuffer)
            {
                DXASSERT(false, "VertexBuffer can not be empty");
            }
            return VertexBuffer.Get();
        }

        // Release the resource, that class can not be later used!
        _NODISCARD auto ReleaseResource()
        {
            if (!VertexBuffer)
            {
                DXASSERT(false, "VertexBuffer can not be empty");
            }
            return std::move(VertexBuffer);
        }
        
        // Resource
        ComPtr<DXResource>  VertexBuffer;
    };

    // VERTEX HELPER
    struct INDEX_HELPER
    {
        DISABLE_COPY(INDEX_HELPER)

        // Ctor
        INDEX_HELPER(DXDevice * Device, const UINT VertexBufferSize,
            D3D12_HEAP_PROPERTIES HEAP_PROPERTY = DX_HEAP_PROPERTY_DEFAULT,
            D3D12_RESOURCE_STATES RESOURCE_STATE = D3D12_RESOURCE_STATE_COMMON,
            LPCWSTR Name = L"IndexBuffer");

        // Create View for Vertex Buffer
        _NODISCARD D3D12_INDEX_BUFFER_VIEW CreateView(const UINT SizeInBytes, DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT);

        // Get pointer
        _NODISCARD auto GetPointer()
        {
            if (!IndexBuffer)
            {
                DXASSERT(false, "IndexBuffer can not be empty");
            }
            return IndexBuffer.Get();
        }

        // Release the resource, that class can not be later used!
        _NODISCARD auto ReleaseResource()
        {
            if (!IndexBuffer)
            {
                DXASSERT(false, "IndexBuffer can not be empty");
            }
            return std::move(IndexBuffer);
        }

        // Resource
        ComPtr<DXResource>  IndexBuffer;
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
        double GetFrameDelta()
        {
            LARGE_INTEGER li;
            QueryPerformanceCounter(&li);
            frameDelta = double(li.QuadPart - lastFrameTime) / timerFrequency;
            if (frameDelta > 0)
                fps = 1000 / frameDelta;
            lastFrameTime = li.QuadPart;
            return frameDelta;
        }
    };

    /***************************
	 ******** FUNCTIONS ********
	 ***************************/
    // Get adapter
    ComPtr<IDXGIAdapter1>	GetAdapter(ComPtr<IDXGIFactory1> pFactory, D3D_FEATURE_LEVEL deviceFeatureLevel, bool useWarpAdapter = false, bool requestHighPerformanceAdapter = false);

    // Creates a Command List. Optionally with Command Allocator and Pipeline State
    [[nodiscard]] DXGraphicsCommandList *				CreateGraphicsCommandList(DXDevice * const Device, CommandListDesc CommandListDesc, DXCommandAllocator * CommandAllocator, DXPipelineState * PipelineState = nullptr);

    // Creates a GraphicsCommandList1 in closed state, using Device4 inside
    [[nodiscard]] DXGraphicsCommandList1 *				CreateGraphicsCommandList1(DXDevice * const Device, CommandListDesc CommandListDesc);

    // Creates a Command Queue
    [[nodiscard]] DXCommandQueue *						CreateCommandQueue(DXDevice * const Device, D3D12_COMMAND_QUEUE_DESC CommandQueueDesc);

    // Creates root constant
    [[nodiscard]] D3D12_ROOT_CONSTANTS					CreateRootConstant(UINT RegisterSpace, UINT ShaderRegister, UINT Num32BitValues);

    // Creates root descriptor
    [[nodiscard]] D3D12_ROOT_DESCRIPTOR					CreateRootDescriptor(UINT RegisterSpace, UINT ShaderRegister);

    // Creates root descriptor
    [[nodiscard]] D3D12_ROOT_DESCRIPTOR_TABLE			CreateRootDescriptorTable(UINT NumDescriptorRanges, const D3D12_DESCRIPTOR_RANGE * DescriptorRange);

    // Creates root parameter/s
    [[nodiscard]] std::vector<D3D12_ROOT_PARAMETER>		CreateRootParameters(std::initializer_list<RootParamHelper> Params);


    // Inline definitions
    inline void ThrowIfFailed(HRESULT hr)
    {
        auto HrToString = [&]
        {
            char s_str[512] = {};
            sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
            return std::string{ s_str };
        };

        if (FAILED(hr))
        {
            //no need to pass argument to lambda
            DXLOG("HRESULT of %08d \n", static_cast<HRESULT>(hr));
            throw std::runtime_error(HrToString());
        }
    }


    // Buffer Byte Size with alignment
    constexpr UINT CalculateConstantBufferByteSize(UINT ByteSize) noexcept
    {
        // Constant buffer size is required to be aligned.
        return (ByteSize + (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT - 1);
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