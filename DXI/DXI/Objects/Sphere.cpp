#include "Sphere.h"

using namespace DirectX;

using TypeOfVertex = VertexPositionTexCoordNormalTangentBitangent;

// The sphere at MIN_DENSITY, written out so the cheapest case costs nothing at runtime,
// the same way Cube and Plane carry their geometry. Three latitude bands and six longitude
// segments, which is 24 triangles.
//
// Un-welded, one set of three vertices per triangle rather than an indexed grid. The frame
// solver below writes a face's normal and tangent into each of its vertices by index, so a
// vertex shared between faces would end up with whichever face came last. Cube and Plane
// get away with an indexed list because every face there owns its four corners.
//
// Generated from the same grid maths Init uses further down, so raising the density past
// this hands over to that generator and the shape does not change character. Do not hand
// edit: regenerate if MIN_DENSITY or the radius ever move.
static VertexPositionTexCoord MinDensityVertexList[] =
{
    // latitude band 0
    {  0.000000f,  0.500000f,  0.000000f,  0.166667f,  0.000000f },
    {  0.216506f,  0.250000f,  0.375000f,  0.166667f,  0.333333f },
    {  0.433013f,  0.250000f,  0.000000f,  0.000000f,  0.333333f },
    {  0.000000f,  0.500000f,  0.000000f,  0.333333f,  0.000000f },
    { -0.216506f,  0.250000f,  0.375000f,  0.333333f,  0.333333f },
    {  0.216506f,  0.250000f,  0.375000f,  0.166667f,  0.333333f },
    {  0.000000f,  0.500000f,  0.000000f,  0.500000f,  0.000000f },
    { -0.433013f,  0.250000f,  0.000000f,  0.500000f,  0.333333f },
    { -0.216506f,  0.250000f,  0.375000f,  0.333333f,  0.333333f },
    {  0.000000f,  0.500000f,  0.000000f,  0.666667f,  0.000000f },
    { -0.216506f,  0.250000f, -0.375000f,  0.666667f,  0.333333f },
    { -0.433013f,  0.250000f,  0.000000f,  0.500000f,  0.333333f },
    {  0.000000f,  0.500000f,  0.000000f,  0.833333f,  0.000000f },
    {  0.216506f,  0.250000f, -0.375000f,  0.833333f,  0.333333f },
    { -0.216506f,  0.250000f, -0.375000f,  0.666667f,  0.333333f },
    {  0.000000f,  0.500000f,  0.000000f,  1.000000f,  0.000000f },
    {  0.433013f,  0.250000f,  0.000000f,  1.000000f,  0.333333f },
    {  0.216506f,  0.250000f, -0.375000f,  0.833333f,  0.333333f },

    // latitude band 1
    {  0.433013f,  0.250000f,  0.000000f,  0.000000f,  0.333333f },
    {  0.216506f,  0.250000f,  0.375000f,  0.166667f,  0.333333f },
    {  0.433013f, -0.250000f,  0.000000f,  0.000000f,  0.666667f },
    {  0.216506f,  0.250000f,  0.375000f,  0.166667f,  0.333333f },
    {  0.216506f, -0.250000f,  0.375000f,  0.166667f,  0.666667f },
    {  0.433013f, -0.250000f,  0.000000f,  0.000000f,  0.666667f },
    {  0.216506f,  0.250000f,  0.375000f,  0.166667f,  0.333333f },
    { -0.216506f,  0.250000f,  0.375000f,  0.333333f,  0.333333f },
    {  0.216506f, -0.250000f,  0.375000f,  0.166667f,  0.666667f },
    { -0.216506f,  0.250000f,  0.375000f,  0.333333f,  0.333333f },
    { -0.216506f, -0.250000f,  0.375000f,  0.333333f,  0.666667f },
    {  0.216506f, -0.250000f,  0.375000f,  0.166667f,  0.666667f },
    { -0.216506f,  0.250000f,  0.375000f,  0.333333f,  0.333333f },
    { -0.433013f,  0.250000f,  0.000000f,  0.500000f,  0.333333f },
    { -0.216506f, -0.250000f,  0.375000f,  0.333333f,  0.666667f },
    { -0.433013f,  0.250000f,  0.000000f,  0.500000f,  0.333333f },
    { -0.433013f, -0.250000f,  0.000000f,  0.500000f,  0.666667f },
    { -0.216506f, -0.250000f,  0.375000f,  0.333333f,  0.666667f },
    { -0.433013f,  0.250000f,  0.000000f,  0.500000f,  0.333333f },
    { -0.216506f,  0.250000f, -0.375000f,  0.666667f,  0.333333f },
    { -0.433013f, -0.250000f,  0.000000f,  0.500000f,  0.666667f },
    { -0.216506f,  0.250000f, -0.375000f,  0.666667f,  0.333333f },
    { -0.216506f, -0.250000f, -0.375000f,  0.666667f,  0.666667f },
    { -0.433013f, -0.250000f,  0.000000f,  0.500000f,  0.666667f },
    { -0.216506f,  0.250000f, -0.375000f,  0.666667f,  0.333333f },
    {  0.216506f,  0.250000f, -0.375000f,  0.833333f,  0.333333f },
    { -0.216506f, -0.250000f, -0.375000f,  0.666667f,  0.666667f },
    {  0.216506f,  0.250000f, -0.375000f,  0.833333f,  0.333333f },
    {  0.216506f, -0.250000f, -0.375000f,  0.833333f,  0.666667f },
    { -0.216506f, -0.250000f, -0.375000f,  0.666667f,  0.666667f },
    {  0.216506f,  0.250000f, -0.375000f,  0.833333f,  0.333333f },
    {  0.433013f,  0.250000f,  0.000000f,  1.000000f,  0.333333f },
    {  0.216506f, -0.250000f, -0.375000f,  0.833333f,  0.666667f },
    {  0.433013f,  0.250000f,  0.000000f,  1.000000f,  0.333333f },
    {  0.433013f, -0.250000f,  0.000000f,  1.000000f,  0.666667f },
    {  0.216506f, -0.250000f, -0.375000f,  0.833333f,  0.666667f },

    // latitude band 2
    {  0.433013f, -0.250000f,  0.000000f,  0.000000f,  0.666667f },
    {  0.216506f, -0.250000f,  0.375000f,  0.166667f,  0.666667f },
    {  0.000000f, -0.500000f,  0.000000f,  0.000000f,  1.000000f },
    {  0.216506f, -0.250000f,  0.375000f,  0.166667f,  0.666667f },
    { -0.216506f, -0.250000f,  0.375000f,  0.333333f,  0.666667f },
    {  0.000000f, -0.500000f,  0.000000f,  0.166667f,  1.000000f },
    { -0.216506f, -0.250000f,  0.375000f,  0.333333f,  0.666667f },
    { -0.433013f, -0.250000f,  0.000000f,  0.500000f,  0.666667f },
    {  0.000000f, -0.500000f,  0.000000f,  0.333333f,  1.000000f },
    { -0.433013f, -0.250000f,  0.000000f,  0.500000f,  0.666667f },
    { -0.216506f, -0.250000f, -0.375000f,  0.666667f,  0.666667f },
    {  0.000000f, -0.500000f,  0.000000f,  0.500000f,  1.000000f },
    { -0.216506f, -0.250000f, -0.375000f,  0.666667f,  0.666667f },
    {  0.216506f, -0.250000f, -0.375000f,  0.833333f,  0.666667f },
    {  0.000000f, -0.500000f,  0.000000f,  0.666667f,  1.000000f },
    {  0.216506f, -0.250000f, -0.375000f,  0.833333f,  0.666667f },
    {  0.433013f, -0.250000f,  0.000000f,  1.000000f,  0.666667f },
    {  0.000000f, -0.500000f,  0.000000f,  0.833333f,  1.000000f },
};

// Un-welded, so the indices are simply 0, 1, 2, ... in order
static TypeOfIndice MinDensityIndices[_countof(MinDensityVertexList)] = {};

// Fills the identity index list once, before main runs, rather than repeating it above
static const bool bMinDensityIndicesReady = []()
{
	for (TypeOfIndice Index = 0; Index < static_cast<TypeOfIndice>(_countof(MinDensityIndices)); ++Index)
	{
		MinDensityIndices[Index] = Index;
	}
	return true;
}();

// Solved the same way Cube and Plane solve theirs
static const auto MinDensityVertices =
	ComputeFaceNormalTangent<VertexPositionTexCoord, TypeOfVertex>(MinDensityVertexList, MinDensityIndices);

namespace
{
	// Matches the unit cube, which spans plus or minus a half on every axis
	constexpr float SPHERE_RADIUS = 0.5f;

	UINT ClampDensity(const UINT Density)
	{
		if (Density < Sphere::MIN_DENSITY)	return Sphere::MIN_DENSITY;
		if (Density > Sphere::MAX_DENSITY)	return Sphere::MAX_DENSITY;
		return Density;
	}
}

Sphere::Sphere(std::wstring Name, const UINT Density)
	: Primitive(std::move(Name))
	, Density(ClampDensity(Density))
{
}

void Sphere::SetDensity(const UINT InDensity) noexcept
{
	Density = ClampDensity(InDensity);
}

UINT Sphere::GetTriangleCountFor(const UINT Density) noexcept
{
	const UINT Stacks = ClampDensity(Density);
	const UINT Slices = Stacks * 2;

	// Two triangles per quad, except the band against each pole where one corner of the
	// quad collapses onto the pole and only a single triangle survives
	return (Stacks - 2) * Slices * 2 + (Slices * 2);
}

HRESULT Sphere::Init(DXDevice * Device, DXGraphicsCommandList * CommandList)
{
	HRESULT hr = Primitive::Init(Device, CommandList);

	const UINT Stacks		= ClampDensity(Density);
	const UINT Slices		= Stacks * 2;

	// The cheapest sphere is already written out, so take it rather than rebuilding it
	if (Stacks == MIN_DENSITY)
	{
		SetNumIndices(static_cast<UINT>(_countof(MinDensityIndices)));

		VertexFactory.Init(Device, CommandList,
			VertexInitData<TypeOfVertex>::CreateVertexInitData(MinDensityVertices, MinDensityIndices, _countof(MinDensityIndices)));

		return hr;
	}

	// One extra column so the seam carries both u of 0 and u of 1. Sharing that column
	// would make the last quad run its u backwards from 1 to 0, and the tangent solve
	// would come out mirrored along the seam.
	const UINT ColumnCount	= Slices + 1;

	std::vector<XMFLOAT3> GridPositions;
	std::vector<XMFLOAT2> GridTexCoords;
	GridPositions.reserve(SIZE_T(Stacks + 1) * ColumnCount);
	GridTexCoords.reserve(SIZE_T(Stacks + 1) * ColumnCount);

	for (UINT StackIdx = 0; StackIdx <= Stacks; ++StackIdx)
	{
		// Phi runs from the +Y pole to the -Y pole, so v of 0 is the top
		const float V		= static_cast<float>(StackIdx) / static_cast<float>(Stacks);
		const float Phi		= V * XM_PI;
		const float Height	= cosf(Phi) * SPHERE_RADIUS;
		const float Ring	= sinf(Phi) * SPHERE_RADIUS;

		for (UINT SliceIdx = 0; SliceIdx < ColumnCount; ++SliceIdx)
		{
			const float U		= static_cast<float>(SliceIdx) / static_cast<float>(Slices);
			const float Theta	= U * XM_2PI;

			GridPositions.emplace_back(Ring * cosf(Theta), Height, Ring * sinf(Theta));
			GridTexCoords.emplace_back(U, V);
		}
	}

	// Every triangle keeps its own three vertices, the same as the mesh loader does. The
	// frame solver writes a face's normal into each of its vertices, so a vertex shared
	// between faces would take the frame of whichever face was written last.
	const UINT TriangleCount = GetTriangleCountFor(Stacks);

	std::vector<TypeOfVertex> Vertices;
	std::vector<TypeOfIndice> Indices;
	Vertices.reserve(SIZE_T(TriangleCount) * 3);
	Indices.reserve(SIZE_T(TriangleCount) * 3);

	// Winding is top left, top right, bottom left. That is what makes the solved normal
	// point away from the centre; the other order produces an inside out sphere that is
	// lit from within. The assert below is what holds it to that.
	const auto EmitTriangle = [&](const UINT A, const UINT B, const UINT C)
	{
		const XMFLOAT3 Position[3]	= { GridPositions[A], GridPositions[B], GridPositions[C] };
		const XMFLOAT2 TexCoord[3]	= { GridTexCoords[A], GridTexCoords[B], GridTexCoords[C] };

		const auto Frame = ComputeTriangleFrame(Position[0], Position[1], Position[2],
												TexCoord[0], TexCoord[1], TexCoord[2]);

		// A sphere centred on the origin has one property worth leaning on: every face
		// normal must point away from the centre. Catching a flipped winding here beats
		// discovering it as a sphere that is dark on the outside.
		const auto Centroid = XMVectorSet((Position[0].x + Position[1].x + Position[2].x) / 3.f,
										(Position[0].y + Position[1].y + Position[2].y) / 3.f,
										(Position[0].z + Position[1].z + Position[2].z) / 3.f,
										0.f);
		DXASSERT(XMVectorGetX(XMVector3Dot(XMLoadFloat4(&Frame.Normal), Centroid)) > 0.f,
				"Sphere face is wound inside out, its normal points at the centre");

		for (UINT Corner = 0; Corner < 3; ++Corner)
		{
			TypeOfVertex Vertex;
			Vertex.Position		= XMFLOAT4(Position[Corner].x, Position[Corner].y, Position[Corner].z, 1.f);
			Vertex.TexCoord		= TexCoord[Corner];
			Vertex.Normal		= Frame.Normal;
			Vertex.Tangent		= Frame.Tangent;
			Vertex.Bitangent	= Frame.Bitangent;

			Indices.push_back(static_cast<TypeOfIndice>(Vertices.size()));
			Vertices.push_back(Vertex);
		}
	};

	for (UINT StackIdx = 0; StackIdx < Stacks; ++StackIdx)
	{
		for (UINT SliceIdx = 0; SliceIdx < Slices; ++SliceIdx)
		{
			const UINT TopLeft		= (StackIdx * ColumnCount) + SliceIdx;
			const UINT TopRight		= TopLeft + 1;
			const UINT BottomLeft	= ((StackIdx + 1) * ColumnCount) + SliceIdx;
			const UINT BottomRight	= BottomLeft + 1;

			// The row against a pole has its whole top or bottom edge collapsed onto the
			// pole, so one of the two triangles has no area and is skipped rather than
			// handed to the tangent solve, which would divide by a zero cross product.
			if (StackIdx != 0)
			{
				EmitTriangle(TopLeft, TopRight, BottomLeft);
			}

			if (StackIdx != Stacks - 1)
			{
				EmitTriangle(TopRight, BottomRight, BottomLeft);
			}
		}
	}

	DXASSERT(Vertices.size() == SIZE_T(TriangleCount) * 3, "Triangle count does not match what was emitted");

	SetNumIndices(static_cast<UINT>(Indices.size()));

	VertexFactory.Init(Device, CommandList,
		VertexInitData<TypeOfVertex>::CreateVertexInitData(std::move(Vertices), std::move(Indices)));

	return hr;
}
