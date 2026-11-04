//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef RENDER_GRAPH_H_INCLUDED
#define RENDER_GRAPH_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"

// Resources
#include "D3D12Buffer.h"
#include "JobLambda.h"

// Usings
// JobBase
using JobBaseRef			= JobBase*;

// Buffer
using BufferRef				= Buffer *;
using BufferRefConst		= Buffer * const;
using BufferConstRef		= Buffer const *;
using BufferConstRefConst	= Buffer const * const;

// Single Threaded Render Graph
class RenderGraph
{
	DISABLE_COPY_MOVE(RenderGraph)

public:

	// Ctors and Dctor
	RenderGraph() = default;
	RenderGraph(ComPtr<DXDevice>& Device, ComPtr<DXGraphicsCommandList>& CommandList);
	~RenderGraph() = default;

	//Initializers functions
	void InitializeFrameResources();

	// Sync point
	void WaitForPreviousFrame();


	// Utility functions
	Buffer CreateBuffer(const BufferDesc& BufferDesc)
	{
		Buffer Temp(BufferDesc);
		return Temp; //RVO
	}

	// Resource Views
	static D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(const	D3D12_GPU_VIRTUAL_ADDRESS VertexBufferGPUVirtualAddress,	const UINT			StrideInBytes,	const UINT SizeInBytes);
	static D3D12_INDEX_BUFFER_VIEW	CreateIndexBufferView(const		D3D12_GPU_VIRTUAL_ADDRESS IndexBufferGPUVirtualAddress,		const DXGI_FORMAT	IndexFormat,	const UINT SizeInBytes);

	// Barriers and Transitions
	static void TransitionBarrier(DXGraphicsCommandList * CommandList, BufferRef Buffer, D3D12_RESOURCE_STATES StateAfter);

	// Add Pass
	template<typename JobLambdaType>
	void AddPass(const wchar_t* PassName, const std::vector<BufferRef>& JobBuffers, JobLambdaType&& Job)
	{
		// Create resources
		for (const BufferRef& BufferRef : JobBuffers)
		{
			Buffers.push_back(BufferRef);
		}

		// create pointer for closure
		JobBaseRef JobBase = new JobLambda<JobLambdaType>(PassName, std::move(Job));

		// Assing pointer to array
		Passes.push_back(JobBase);
	}

	// Execute
	void Execute();


	void BeginFrame();
	void EndFrame();

	// Constants / Misc
	const UINT											BACK_BUFFER_COUNT				= 3;
	const D3D12_COMMAND_LIST_TYPE						COMMAND_LIST_TYPE				= D3D12_COMMAND_LIST_TYPE_DIRECT;
	const D3D_FEATURE_LEVEL								D3D12_FEATURE_LEVEL				= D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;

	// D3D12 Variables
	ComPtr<DXFactory4>									Factory;
	ComPtr<DXDevice>									Device;
	ComPtr<DXCommandQueue>								CommandQueue;
	ComPtr<DXGraphicsCommandList>						CommandList;
	std::vector<ComPtr<DXCommandAllocator>>				CommandAllocators;

	// D3D12 Synchronization CPU<->GPU
	std::vector<ComPtr<DXFence>>						Fences;
	std::vector<UINT64>									FenceValue;
	HANDLE												FenceEvent;

	// D3D12 SwapChain
	ComPtr<IDXGISwapChain3>								SwapChain;
	UINT												CurrentFrameIdx;

	// D3D12 Frame Buffer Render Target
	ComPtr<DXDescriptorHeap>							RtvHeap;
	std::vector<ComPtr<DXResource>>						RenderTargets;
	UINT												RtvIncrementDescriptorSize;

	// Resources
	std::vector<BufferRef>								Buffers;
	std::vector<JobBaseRef>								Passes;

};


#endif /* RENDER_GRAPH_H_INCLUDED */