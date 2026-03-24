#include "Vertex.h"

VertexFactory::VertexFactory(const VertexType Type)
	: Type(Type)
{
	const uint64_t VertexSizeInBytes = VertexTypeSizes[static_cast<uint32_t>(Type)];	// Get the size of the vertex structure based on the specified vertex type
}

HRESULT VertexFactory::Init(DXDevice * Device, DXGraphicsCommandList* CommandList, VertexInitData InitData)
{
    // Usings
    using Helpers::ThrowIfFailed;

    // HRESULT, just in case...
	HRESULT hr = S_OK;

    // Asserts
	DXASSERT(InitData.VertexBufferSize > 0, "Vertex buffer size must be greater than 0");
	DXASSERT(!InitData.VertexData.empty(), "Vertex data must not be empty");
	DXASSERT(InitData.VertexBufferSize == InitData.VertexData.size(), "Vertex buffer size must match the size of vertex data");
	DXASSERT(InitData.Type == Type, "Vertex type in InitData must match the VertexType of this Vertex instance");
	DXASSERT((static_cast<size_t>(InitData.Type) % InitData.VertexData.size()) == 0, "Vertex buffer size must be a multiple of the vertex type size");

    /***************************
    ****** VERTEX BUFFER ******
    ***************************/
    {
        constexpr auto StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        constexpr auto StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        // GPU Vertex
        Helpers::VERTEX_HELPER VertexGPU(Device,
            InitData.VertexBufferSize,
            DX_HEAP_PROPERTY_DEFAULT,
            D3D12_RESOURCE_STATE_COMMON,
            L"VertexGPU");

        // Upload Vertex
        Helpers::VERTEX_HELPER VertexUploadToGPU(Device,
            InitData.VertexBufferSize,
            DX_HEAP_PROPERTY_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            L"VertexUploadToGPU");

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the vertex buffer.
        D3D12_SUBRESOURCE_DATA VertexSubData = {};
        VertexSubData.pData        = reinterpret_cast<const void*>(InitData.VertexData.data());
        VertexSubData.RowPitch     = InitData.VertexBufferSize;
        VertexSubData.SlicePitch   = VertexSubData.RowPitch;

        // Update Subresource
        UpdateSubresources(CommandList, VertexGPU.GetPointer(), VertexUploadToGPU.GetPointer(), 0, 0, 1, &VertexSubData);

        // transition the vertex buffer data from copy destination state to vertex buffer state
        const auto VertexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexGPU.GetPointer(), StateBefore, StateAfter);
        CommandList->ResourceBarrier(1, &VertexCmdListBarrier);

        // Release the resources
        VertexBufferView    = VertexGPU.CreateView(VertexTypeSizes[static_cast<uint32_t>(Type)], InitData.VertexBufferSize);
        VertexBuffer        = VertexGPU.ReleaseResource();
        VertexBufferUpload  = VertexUploadToGPU.ReleaseResource();
    }

	return hr;
}
