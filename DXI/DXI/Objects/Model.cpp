#include "Model.h"

#include "ObjLoader.h"

using TypeOfVertex = VertexPositionUVNormalTangentBitangent;

Model::Model(std::wstring Name, std::wstring MeshPath)
	: Primitive(std::move(Name))
	, MeshPath(std::move(MeshPath))
{
}

Model::Model(std::wstring Name, LoadedMesh Mesh)
	: Primitive(std::move(Name))
	, PreloadedMesh(std::move(Mesh))
{
}

HRESULT Model::Init(DXDevice * Device, DXGraphicsCommandList * CommandList)
{
	HRESULT hr = Primitive::Init(Device, CommandList);

	DXASSERT(!MeshPath.empty() || !PreloadedMesh.Vertices.empty(), "Model needs either a mesh path or geometry before it can be initialized");

	auto Mesh = !PreloadedMesh.Vertices.empty() ? std::move(PreloadedMesh) : LoadObjFromFile(MeshPath);
	PreloadedMesh = LoadedMesh();

	const auto NumIndices = static_cast<UINT>(Mesh.Indices.size());
	SetNumIndices(NumIndices);

	DXLOG("Loaded model with %u vertices and %u indices\n", static_cast<UINT>(Mesh.Vertices.size()), NumIndices)

	VertexFactory.Init(Device, CommandList,
		VertexInitData<TypeOfVertex>::CreateVertexInitData(std::move(Mesh.Vertices), std::move(Mesh.Indices)));

	return hr;
}
