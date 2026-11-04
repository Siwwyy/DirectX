//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_BUFFER_H_INCLUDED
#define D3D12_BUFFER_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"

// Type of buffers to the RHI
enum class BufferType : UINT
{
	VertexBuffer		= 0,
	IndexBuffer			= 1,
	StructuredBuffer	= 2,
};

// Buffer Descritor
struct BufferDesc
{
	BufferDesc() = default;
	~BufferDesc() = default;

	// Actual number of elements in buffer
	UINT NumElements = 1;

	// Stride for next element (size of single elem)
	UINT BytesPerElement = 1;

	// Buffer Type, State, Heap
	BufferType				BufferType		= ::BufferType::VertexBuffer;
	D3D12_HEAP_PROPERTIES	HeapProperty	= DX_HEAP_PROPERTY_DEFAULT;
	D3D12_RESOURCE_STATES	State			= D3D12_RESOURCE_STATE_COMMON;

	// Resource Name
	std::wstring Name = L"VertexBuffer";

	// Create Buffer Desc Helper
	static BufferDesc CreateBufferDesc(INT NumElements, UINT BytesPerElement, ::BufferType BufferType, D3D12_HEAP_PROPERTIES HeapProperty, D3D12_RESOURCE_STATES State, const std::wstring& Name);
};

// Single Threaded Render Graph
class Buffer
{
	//DISABLE_COPY(Buffer)

public:

	// Ctors and Dctor
	Buffer() = default;
	~Buffer() = default;
	Buffer(BufferDesc Desc);

	// Functions
	void InitializeUnderlayingResource(DXDevice * Device);

	// Underlaying Resource
	ComPtr<DXResource>					Resource;

	// Buffer Desc
	BufferDesc							Desc;
private:
	

	
};

#endif // D3D12_BUFFER_H_INCLUDED
