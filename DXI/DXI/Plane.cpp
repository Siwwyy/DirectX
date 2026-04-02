#include "Plane.h"


// Vertices/Index buffers etc.
static const VertexPositionTexCoord VertexList[] =
{

	//{ -0.5f,  0.5f, 0.5f , 1.0f, 1.0f, 1.0f, 1.0f}, // top left
	//{ 0.5f , -0.5f, 0.5f , 1.0f, 1.0f, 0.0f, 1.0f},	// bottom right
	//{ -0.5f, -0.5f, 0.5f , 1.0f, 0.0f, 0.0f, 1.0f}, // bottom left
	//{  0.5f,  0.5f, 0.5f , 0.0f, 0.0f, 1.0f, 1.0f}, // top right

	{ -0.5f,  0.5f, 0.5f , 0.0f, 0.0f }, // top left
	{ 0.5f , -0.5f, 0.5f , 1.0f, 1.0f }, // bottom right
	{ -0.5f, -0.5f, 0.5f , 0.0f, 1.0f }, // bottom left
	{  0.5f,  0.5f, 0.5f , 1.0f, 0.0f }  // top right
};

static constexpr DWORD IndicesList[] =
{
	//// front face
	//0, 1, 2, // first triangle
	//0, 3, 1, // second triangle

	//// left face
	//4, 5, 6, // first triangle
	//4, 7, 5, // second triangle

	//// right face
	//8, 9, 10, // first triangle
	//8, 11, 9, // second triangle

	//// back face
	//12, 13, 14, // first triangle
	//12, 15, 13, // second triangle

	//// top face
	//16, 17, 18, // first triangle
	//16, 19, 17, // second triangle

	//// bottom face
	//20, 21, 22, // first triangle
	//20, 23, 21, // second triangle

	///////////////////////////////
	0, 1, 2,
	0, 3, 1
	// Its order is because its clock-wise when using Triangle List topology
	// See: https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-primitive-topologies

	///////////////////////////////
};



//static const auto TexNormalVertexList           = ComputeFaceNormal< VertexPositionTexCoord, VertexPositionTexCoordNormal>(VertexList, IndicesList);
static constexpr const UINT VertexBufferSize    = sizeof(VertexList);
static constexpr UINT IndexBufferSize           = sizeof(IndicesList);
static constexpr UINT PlaneNumIndices           = IndexBufferSize / sizeof(DWORD);
//
//static constexpr const UINT VertexBufferSize    = sizeof(TexNormalVertexList);
//static constexpr UINT IndexBufferSize           = sizeof(IndicesList);
//static constexpr UINT PlaneNumIndices           = IndexBufferSize / sizeof(DWORD);


// Ctors
Plane::Plane()
    : Primitive()
{ }

Plane::Plane(const DirectX::XMFLOAT4 InitRot,
            const DirectX::XMFLOAT4 InitPosition,
            const XMFLOAT4 InitScale)
    : Primitive(InitRot, InitPosition, InitScale)
{ }

// Functions
HRESULT Plane::Init(DXDevice* Device, DXGraphicsCommandList * CommandList)
{
    using Helpers::ThrowIfFailed;
    HRESULT hr = Primitive::Init(Device, CommandList);

    // Set neccessary values
    SetNumIndices(PlaneNumIndices);


    //***************************
    //****** VERTEX BUFFER ******
    //***************************/

  //  {
		//VertexPositionTexCoordNormal VertexPTNList[std::size(VertexList)]{};

  //      VertexInitData VertexInitData;
		//VertexInitData.Type             = VertexType::PositionTexcoordNormal;
		//VertexInitData.VertexBufferSize = VertexBufferSize;

		////VertexFactory.Init(Device, CommandList, TexNormalVertexList, VertexBufferSize);
  //  }

    /***************************
    ****** VERTEX BUFFER ******
    ***************************/
    {
        constexpr auto StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        constexpr auto StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        // GPU Vertex
        Helpers::VERTEX_HELPER VertexGPU(Device,
            VertexBufferSize,
            DX_HEAP_PROPERTY_DEFAULT,
            D3D12_RESOURCE_STATE_COMMON,
            L"VertexGPU");

        // Upload Vertex
        Helpers::VERTEX_HELPER VertexUploadToGPU(Device,
            VertexBufferSize,
            DX_HEAP_PROPERTY_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            L"VertexUploadToGPU");

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the vertex buffer.
        D3D12_SUBRESOURCE_DATA VertexData = {};
        VertexData.pData = reinterpret_cast<const void*>(VertexList);
        VertexData.RowPitch = VertexBufferSize;
        VertexData.SlicePitch = VertexData.RowPitch;

        // Update Subresource
        UpdateSubresources(CommandList, VertexGPU.GetPointer(), VertexUploadToGPU.GetPointer(), 0, 0, 1, &VertexData);

        // transition the vertex buffer data from copy destination state to vertex buffer state
        const auto VertexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexGPU.GetPointer(), StateBefore, StateAfter);
        CommandList->ResourceBarrier(1, &VertexCmdListBarrier);

        // Release the resources
        VertexFactory.VertexBufferView = VertexGPU.CreateView(VertexTypeSizes[static_cast<size_t>(VertexType::PositionTexCoord)], VertexBufferSize);
        VertexFactory.VertexBuffer          = VertexGPU.ReleaseResource();
        VertexFactory.VertexBufferUpload    = VertexUploadToGPU.ReleaseResource();
    }


    /***************************
     ****** INDEX BUFFER *******
     ***************************/
    {
        // GPU Vertex
        constexpr auto StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        constexpr auto StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        Helpers::INDEX_HELPER IndexGPU(Device,
            IndexBufferSize,
            DX_HEAP_PROPERTY_DEFAULT,
            D3D12_RESOURCE_STATE_COMMON,
            L"IndexGPU");

        // Upload Vertex
        Helpers::INDEX_HELPER IndexUploadToGPU(Device,
            IndexBufferSize,
            DX_HEAP_PROPERTY_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            L"IndexUploadToGPU");

        // store index buffer in upload heap
        D3D12_SUBRESOURCE_DATA IndexData = {};
        IndexData.pData = reinterpret_cast<const void*>(IndicesList); // pointer to our index array
        IndexData.RowPitch = IndexBufferSize;						 // size of all our index buffer
        IndexData.SlicePitch = IndexData.RowPitch;					 // also the size of our index buffer

        // Update Subresource
        UpdateSubresources<1>(CommandList, IndexGPU.GetPointer(), IndexUploadToGPU.GetPointer(), 0, 0, 1, &IndexData);

        // transition the vertex buffer data from copy destination state to vertex buffer state
        const auto IndexCmdListBarrier = CD3DX12_RESOURCE_BARRIER::Transition(IndexGPU.GetPointer(), StateBefore, StateAfter);
        CommandList->ResourceBarrier(1, &IndexCmdListBarrier);

        // Release the resources
        IndexBufferView = IndexGPU.CreateView(IndexBufferSize, DXGI_FORMAT_R32_UINT);
        IndexBuffer = IndexGPU.ReleaseResource();
        IndexBufferUpload = IndexUploadToGPU.ReleaseResource();
    }

    return hr;
}


