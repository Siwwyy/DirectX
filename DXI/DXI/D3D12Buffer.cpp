#include "D3D12Buffer.h"

// BufferDesc
BufferDesc BufferDesc::CreateBufferDesc(INT NumElements, UINT BytesPerElement, ::BufferType BufferType, D3D12_HEAP_PROPERTIES HeapProperty, D3D12_RESOURCE_STATES State, const std::wstring& Name)
{
	BufferDesc Desc;
	Desc.NumElements		= NumElements;
	Desc.BytesPerElement	= BytesPerElement;
	Desc.BufferType			= BufferType;
	Desc.HeapProperty		= HeapProperty;
	Desc.State				= State;
	Desc.Name				= Name;
	return Desc;
}

// Buffer
Buffer::Buffer(BufferDesc Desc)
	: Desc(Desc)
{ }

// InitializeUnderlayingResource
void Buffer::InitializeUnderlayingResource(DXDevice * Device)
{
	const UINT BufferSize	= Desc.BytesPerElement * Desc.NumElements;
	if (Desc.BufferType == BufferType::VertexBuffer)
	{
		Helpers::VERTEX_HELPER Vertex(Device,
			BufferSize,
			Desc.HeapProperty,
			Desc.State,
			Desc.Name.c_str());

		// Release the resources
		Resource = Vertex.ReleaseResource();
	}
	else if (Desc.BufferType == BufferType::IndexBuffer)
	{
		Helpers::INDEX_HELPER Index(Device,
			BufferSize,
			Desc.HeapProperty,
			Desc.State,
			Desc.Name.c_str());

		// Release the resources
		Resource = Index.ReleaseResource();
	}
	else // BufferType::StructuredBuffer
	{
		// Not supported yet
	}
}
