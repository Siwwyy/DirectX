#include "Cube.h"

//Vertex VertexList[] =
//{
//    // front face
//    { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//    {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//    { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//    {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//    // right side face
//    {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//    {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//    {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//    // left side face
//    { -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//    { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//    { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//    { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//    // back face
//    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//    { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//    {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//    { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//    // top face
//    { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//    { 0.5f,   0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//    { 0.5f,   0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//    { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//
//    // bottom face
//    {  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
//    { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
//    {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
//    { -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
//};

//static TexVertex VertexList[] =
//{
//    // front face
//    { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
//    {  0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
//    { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
//    {  0.5f,  0.5f, -0.5f, 1.0f, 0.0f },
//
//    // right side face
//    {  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
//    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
//    {  0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
//    {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
//
//    // left side face
//    { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
//    { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
//    { -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
//    { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },
//
//    // back face
//    {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
//    { -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
//    {  0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
//    { -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
//
//    // top face
//    { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
//    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
//    {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
//    { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
//
//    // bottom face
//    {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
//    { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
//    {  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
//    { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
//};

static VertexPositionTexCoord VertexList[] =
{
    // front face
    { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

    // right side face
    {  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

    // left side face
    { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

    // back face
    {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

    // top face
    { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

    // bottom face
    {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
};


// a quad (2 triangles)
static DWORD IndicesList[] =
{
    // front face
    0, 1, 2, // first triangle
    0, 3, 1, // second triangle

    // left face
    4, 5, 6, // first triangle
    4, 7, 5, // second triangle

    // right face
    8, 9, 10, // first triangle
    8, 11, 9, // second triangle

    // back face
    12, 13, 14, // first triangle
    12, 15, 13, // second triangle

    // top face
    16, 17, 18, // first triangle
    16, 19, 17, // second triangle

    // bottom face
    20, 21, 22, // first triangle
    20, 23, 21, // second triangle
};


//static constexpr UINT VertexBufferSize  = sizeof(VertexList);
//static constexpr UINT IndexBufferSize   = sizeof(IndicesList);
//static constexpr UINT CubeNumIndices    = IndexBufferSize / sizeof(DWORD);

static const auto TexNormalVertexList   = ComputeFaceNormal<VertexPositionTexCoord, VertexPositionTexCoordNormal>(VertexList, IndicesList);
static constexpr UINT VertexBufferSize  = sizeof(TexNormalVertexList);
static constexpr UINT IndexBufferSize   = sizeof(IndicesList);
static constexpr UINT CubeNumIndices    = IndexBufferSize / sizeof(DWORD);

// Ctors
Cube::Cube()
    : Primitive()
{ }

// Functions
HRESULT Cube::Init(DXDevice * Device, DXGraphicsCommandList* CommandList)
{
    using Helpers::ThrowIfFailed;
    HRESULT hr = Primitive::Init(Device, CommandList);

    // Set neccessary values
    SetNumIndices(CubeNumIndices);

	// Init per vertex data
	VertexInitData<VertexPositionTexCoordNormal> InitData;
	InitData.Type               = VertexType::PositionTexCoordNormal;
	InitData.VertexBufferSize   = VertexBufferSize;
	InitData.IndexBufferSize    = IndexBufferSize;

	// Maybe rethink that
    std::copy(TexNormalVertexList.begin(), TexNormalVertexList.end(), InitData.VertexData.begin());
	std::copy(IndicesList, IndicesList + CubeNumIndices, InitData.IndexData.begin());

	// Init vertex factory
    VertexFactory.Init(Device, CommandList, std::move(InitData));

    return hr;
}
