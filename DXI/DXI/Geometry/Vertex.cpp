#include "Vertex.h"

Vertex::Vertex(const VertexType Type)
	: Type(Type)
{

	const uint64_t VertexSizeInBytes = VertexTypeSizes[static_cast<uint32_t>(Type)];	// Get the size of the vertex structure based on the specified vertex type
	VertexData.resize(VertexSizeInBytes);												// Initialize vertex data buffer based on the specified vertex type
}

HRESULT Vertex::Init(DXDevice * Device, DXGraphicsCommandList* CommandList)
{
	HRESULT hr = S_OK;
	return hr;
}
