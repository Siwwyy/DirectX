
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


// Disables base default constructor of class
#define DISABLE_DEFAULT_CTOR(CLASS_NAME)                    \
		CLASS_NAME() = delete;                              \

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
            char MsgBuffer[1024]{};                         \
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
#define DX_IDENTITY_SCALE3          XMFLOAT3(1.f, 1.f, 1.f)
#define DX_IDENTITY_ROTATE3         XMFLOAT3(0.f, 0.f, 0.f)
#define DX_IDENTITY_TRANSFORM3      XMFLOAT3(0.f, 0.f, 0.f)

#define DX_IDENTITY_SCALE4          XMFLOAT4(1.f, 1.f, 1.f, 1.f)
#define DX_IDENTITY_ROTATE4         XMFLOAT4(0.f, 0.f, 0.f, 1.f)
#define DX_IDENTITY_TRANSFORM4      XMFLOAT4(0.f, 0.f, 0.f, 1.f)


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
            ZeroMemory(&temp, sizeof(temp));
            temp.ParameterType              = ParameterType;
            temp.ShaderVisibility           = ShaderVisibility;
            temp.Constants                  = ActiveField.Constants;
            temp.Descriptor                 = ActiveField.Descriptor;
            temp.Descriptor                 = ActiveField.Descriptor;
            temp.DescriptorTable            = ActiveField.DescriptorTable;
            return temp;
        }

    private:
        union /* NoName */
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
            ZeroMemory(&temp, sizeof(temp));
            temp.Count      = count;
            temp.Quality    = quality;
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

    // Helper class for animation and simulation timing.
    class StepTimer
    {
    public:
        StepTimer() :
            m_elapsedTicks(0),
            m_totalTicks(0),
            m_leftOverTicks(0),
            m_frameCount(0),
            m_framesPerSecond(0),
            m_framesThisSecond(0),
            m_qpcSecondCounter(0),
            m_isFixedTimeStep(false),
            m_targetElapsedTicks(TicksPerSecond / 60)
        {
            QueryPerformanceFrequency(&m_qpcFrequency);
            QueryPerformanceCounter(&m_qpcLastTime);

            // Initialize max delta to 1/10 of a second.
            m_qpcMaxDelta = m_qpcFrequency.QuadPart / 10;
        }

        // Get elapsed time since the previous Update call.
        UINT64 GetElapsedTicks() const { return m_elapsedTicks; }
        double GetElapsedSeconds() const { return TicksToSeconds(m_elapsedTicks); }

        // Get total time since the start of the program.
        UINT64 GetTotalTicks() const { return m_totalTicks; }
        double GetTotalSeconds() const { return TicksToSeconds(m_totalTicks); }

        // Get total number of updates since start of the program.
        UINT32 GetFrameCount() const { return m_frameCount; }

        // Get the current framerate.
        UINT32 GetFramesPerSecond() const { return m_framesPerSecond; }

        // Set whether to use fixed or variable timestep mode.
        void SetFixedTimeStep(bool isFixedTimestep) { m_isFixedTimeStep = isFixedTimestep; }

        // Set how often to call Update when in fixed timestep mode.
        void SetTargetElapsedTicks(UINT64 targetElapsed) { m_targetElapsedTicks = targetElapsed; }
        void SetTargetElapsedSeconds(double targetElapsed) { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

        // Integer format represents time using 10,000,000 ticks per second.
        static const UINT64 TicksPerSecond = 10000000;

        static double TicksToSeconds(UINT64 ticks) { return static_cast<double>(ticks) / TicksPerSecond; }
        static UINT64 SecondsToTicks(double seconds) { return static_cast<UINT64>(seconds * TicksPerSecond); }

        // After an intentional timing discontinuity (for instance a blocking IO operation)
        // call this to avoid having the fixed timestep logic attempt a set of catch-up 
        // Update calls.

        void ResetElapsedTime()
        {
            QueryPerformanceCounter(&m_qpcLastTime);

            m_leftOverTicks = 0;
            m_framesPerSecond = 0;
            m_framesThisSecond = 0;
            m_qpcSecondCounter = 0;
        }

        typedef void(*LPUPDATEFUNC) (void);

        // Update timer state, calling the specified Update function the appropriate number of times.
        void Tick(LPUPDATEFUNC update = nullptr)
        {
            // Query the current time.
            LARGE_INTEGER currentTime;

            QueryPerformanceCounter(&currentTime);

            UINT64 timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

            m_qpcLastTime = currentTime;
            m_qpcSecondCounter += timeDelta;

            // Clamp excessively large time deltas (e.g. after paused in the debugger).
            if (timeDelta > m_qpcMaxDelta)
            {
                timeDelta = m_qpcMaxDelta;
            }

            // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
            timeDelta *= TicksPerSecond;
            timeDelta /= m_qpcFrequency.QuadPart;

            UINT32 lastFrameCount = m_frameCount;

            if (m_isFixedTimeStep)
            {
                // Fixed timestep update logic

                // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
                // the clock to exactly match the target value. This prevents tiny and irrelevant errors
                // from accumulating over time. Without this clamping, a game that requested a 60 fps
                // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
                // accumulate enough tiny errors that it would drop a frame. It is better to just round 
                // small deviations down to zero to leave things running smoothly.

                if (abs(static_cast<int>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
                {
                    timeDelta = m_targetElapsedTicks;
                }

                m_leftOverTicks += timeDelta;

                while (m_leftOverTicks >= m_targetElapsedTicks)
                {
                    m_elapsedTicks = m_targetElapsedTicks;
                    m_totalTicks += m_targetElapsedTicks;
                    m_leftOverTicks -= m_targetElapsedTicks;
                    m_frameCount++;

                    if (update)
                    {
                        update();
                    }
                }
            }
            else
            {
                // Variable timestep update logic.
                m_elapsedTicks = timeDelta;
                m_totalTicks += timeDelta;
                m_leftOverTicks = 0;
                m_frameCount++;

                if (update)
                {
                    update();
                }
            }

            // Track the current framerate.
            if (m_frameCount != lastFrameCount)
            {
                m_framesThisSecond++;
            }

            if (m_qpcSecondCounter >= static_cast<UINT64>(m_qpcFrequency.QuadPart))
            {
                m_framesPerSecond = m_framesThisSecond;
                m_framesThisSecond = 0;
                m_qpcSecondCounter %= m_qpcFrequency.QuadPart;
            }
        }

    private:
        // Source timing data uses QPC units.
        LARGE_INTEGER m_qpcFrequency;
        LARGE_INTEGER m_qpcLastTime;
        UINT64 m_qpcMaxDelta;

        // Derived timing data uses a canonical tick format.
        UINT64 m_elapsedTicks;
        UINT64 m_totalTicks;
        UINT64 m_leftOverTicks;

        // Members for tracking the framerate.
        UINT32 m_frameCount;
        UINT32 m_framesPerSecond;
        UINT32 m_framesThisSecond;
        UINT64 m_qpcSecondCounter;

        // Members for configuring fixed timestep mode.
        bool m_isFixedTimeStep;
        UINT64 m_targetElapsedTicks;
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
    [[nodiscard]] D3D12_DESCRIPTOR_RANGE			    CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE RangeType, UINT NumDescriptors, UINT BaseShaderRegister, UINT RegisterSpace, UINT OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

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