
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Helpers.h"

// Using namespaces
using Helpers::ThrowIfFailed;
using Helpers::RootParamHelper;
using Helpers::CommandListDesc;

/***************************
 ****** HELPER STRUCTS *****
 ***************************/

// Vertex Helper
Helpers::VERTEX_HELPER::VERTEX_HELPER(DXDevice * Device, const UINT VertexBufferSize, D3D12_HEAP_PROPERTIES HEAP_PROPERTY, D3D12_RESOURCE_STATES RESOURCE_STATE, LPCWSTR Name)
{
	// Create vertex buffer
	const auto VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY,								        // a default heap
		D3D12_HEAP_FLAG_NONE,								// no flags | TODO MAKE FLAGS IN FUTURE
		&VertexBufferDesc,									// resource description for a buffer
		RESOURCE_STATE,										// we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
		nullptr,											// optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&VertexBuffer)));
	NAME_D3D12_OBJECT(VertexBuffer, Name);
	
}

D3D12_VERTEX_BUFFER_VIEW Helpers::VERTEX_HELPER::CreateView(const UINT StrideInBytes, const UINT SizeInBytes)
{
	if (!VertexBuffer)
	{
		DXASSERT(false, "VertexBuffer can not be empty");
		return D3D12_VERTEX_BUFFER_VIEW();
	}
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes	= StrideInBytes;
	VertexBufferView.SizeInBytes	= SizeInBytes;
	return VertexBufferView;
}

// Index helper
Helpers::INDEX_HELPER::INDEX_HELPER(DXDevice * Device, const UINT IndexBufferSize, D3D12_HEAP_PROPERTIES HEAP_PROPERTY, D3D12_RESOURCE_STATES RESOURCE_STATE, LPCWSTR Name)
{
	// create default heap to hold index buffer
	const auto IndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&HEAP_PROPERTY,								// a default heap
		D3D12_HEAP_FLAG_NONE,						// no flags
		&IndexBufferDesc,							// resource description for a buffer
		RESOURCE_STATE,								// start in the copy destination state
		nullptr,									// optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&IndexBuffer)));
	NAME_D3D12_OBJECT(IndexBuffer, Name);
}

D3D12_INDEX_BUFFER_VIEW Helpers::INDEX_HELPER::CreateView(const UINT SizeInBytes, DXGI_FORMAT IndexFormat)
{
	if (!IndexBuffer)
	{
		DXASSERT(false, "IndexBuffer can not be empty");
		return D3D12_INDEX_BUFFER_VIEW();
	}
	// create a index buffer view for the vertices. We get the GPU memory address to the index pointer using the GetGPUVirtualAddress() method later.
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	IndexBufferView.BufferLocation	= IndexBuffer->GetGPUVirtualAddress();
	IndexBufferView.Format			= IndexFormat; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	IndexBufferView.SizeInBytes		= SizeInBytes;
	return IndexBufferView;
}


/***************************
 ******** FUNCTIONS ********
 ***************************/

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

DXGraphicsCommandList* Helpers::CreateGraphicsCommandList(
	DXDevice* const Device,
	CommandListDesc CommandListDesc,
	DXCommandAllocator * CommandAllocator,
	DXPipelineState * PipelineState)
{
	DXGraphicsCommandList * commandList = nullptr;

	ThrowIfFailed(Device->CreateCommandList(CommandListDesc.NodeMask, CommandListDesc.Type, CommandAllocator, PipelineState, IID_PPV_ARGS(&commandList)));

	return commandList;
}

DXGraphicsCommandList1* Helpers::CreateGraphicsCommandList1(
	DXDevice* const Device,
	CommandListDesc CommandListDesc)
{
	DXGraphicsCommandList1 * commandList1 = nullptr;
	DXDevice4* device4 = nullptr;	//Use Device4 to create a command list with closed state

	Device->QueryInterface(IID_PPV_ARGS(&device4));	//Make device4 from device by quering the interface
	ThrowIfFailed(device4->CreateCommandList1(CommandListDesc.NodeMask, CommandListDesc.Type, CommandListDesc.Flags, IID_PPV_ARGS(&commandList1)));

	return commandList1;
}

DXCommandQueue * Helpers::CreateCommandQueue(DXDevice * const Device, D3D12_COMMAND_QUEUE_DESC CommandQueueDesc)
{
	DXCommandQueue * CommandQueue = nullptr;
	ThrowIfFailed(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue)));
	return CommandQueue;
}

D3D12_ROOT_CONSTANTS Helpers::CreateRootConstant(UINT RegisterSpace, UINT ShaderRegister, UINT Num32BitValues)
{
	D3D12_ROOT_CONSTANTS temp = {};
	temp.RegisterSpace = RegisterSpace;
	temp.ShaderRegister = ShaderRegister;
	temp.Num32BitValues = Num32BitValues;
	return temp; //RVO, no move needed (Probably)
}

D3D12_ROOT_DESCRIPTOR Helpers::CreateRootDescriptor(UINT RegisterSpace, UINT ShaderRegister)
{
	D3D12_ROOT_DESCRIPTOR temp = {};
	temp.RegisterSpace = RegisterSpace;
	temp.ShaderRegister = ShaderRegister;
	return temp; //RVO, no move needed (Probably)
}

D3D12_ROOT_DESCRIPTOR_TABLE Helpers::CreateRootDescriptorTable(UINT NumDescriptorRanges, const D3D12_DESCRIPTOR_RANGE * DescriptorRange)
{
	D3D12_ROOT_DESCRIPTOR_TABLE temp = {};
	temp.NumDescriptorRanges = NumDescriptorRanges;
	temp.pDescriptorRanges = DescriptorRange;
	return temp; //RVO, no move needed (Probably)
}

std::vector<D3D12_ROOT_PARAMETER> Helpers::CreateRootParameters(std::initializer_list<RootParamHelper> Params)
{
	std::vector<D3D12_ROOT_PARAMETER> temp;
	for (const Helpers::RootParamHelper& param : Params)
	{
		temp.push_back(param.CreateRootParameter());
	}
	return temp; //initializer list<D3D12_ROOT_PARAMETER>
}