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



static const auto TexNormalVertexList           = ComputeFaceNormal< VertexPositionTexCoord, VertexPositionTexCoordNormal>(VertexList, IndicesList);
static constexpr const UINT VertexBufferSize    = sizeof(TexNormalVertexList);
static constexpr UINT IndexBufferSize           = sizeof(IndicesList);
static constexpr UINT PlaneNumIndices           = IndexBufferSize / sizeof(DWORD);


// Ctors
Plane::Plane()
    : Primitive()
{ }

// Functions
HRESULT Plane::Init(DXDevice* Device, DXGraphicsCommandList * CommandList)
{
    using Helpers::ThrowIfFailed;
    HRESULT hr = Primitive::Init(Device, CommandList);

    // Set neccessary values
    SetNumIndices(PlaneNumIndices);

    // Init per vertex data
    VertexInitData<VertexPositionTexCoordNormal> InitData;
    InitData.Type = VertexType::PositionTexCoordNormal;
    InitData.VertexBufferSize = VertexBufferSize;
    InitData.IndexBufferSize = IndexBufferSize;

    // Maybe rethink that
    std::copy(TexNormalVertexList.begin(), TexNormalVertexList.end(), InitData.VertexData.begin());
    std::copy(IndicesList, IndicesList + PlaneNumIndices, InitData.IndexData.begin());

    // Init vertex factory
    VertexFactory.Init(Device, CommandList, std::move(InitData));

    return hr;
}


