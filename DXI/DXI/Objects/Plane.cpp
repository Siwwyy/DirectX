#include "Plane.h"


// Vertices/Index buffers etc.
static const VertexPositionTexCoord VertexList[] =
{
	{ -0.5f,  0.5f, 0.5f , 0.0f, 0.0f }, // top left
	{ 0.5f , -0.5f, 0.5f , 1.0f, 1.0f }, // bottom right
	{ -0.5f, -0.5f, 0.5f , 0.0f, 1.0f }, // bottom left
	{  0.5f,  0.5f, 0.5f , 1.0f, 0.0f }  // top right
};

static constexpr DWORD IndicesList[] =
{
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

Plane::Plane(std::wstring Name)
    : Primitive(Name)
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