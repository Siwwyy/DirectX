#include "Model.h"

#include "../Geometry/ObjLoader.h"

using TypeOfVertex = VertexPositionTexCoordNormalTangentBitangent;

Model::Model(std::wstring Name, std::wstring MeshPath)
	: Primitive(std::move(Name))
	, MeshPath(std::move(MeshPath))
{
}

HRESULT Model::Init(DXDevice * Device, DXGraphicsCommandList * CommandList)
{
	HRESULT hr = Primitive::Init(Device, CommandList);

	DXASSERT(!MeshPath.empty(), "Model needs a mesh path before it can be initialized");

	auto Mesh = LoadObjFromFile(MeshPath);

	const auto NumIndices = static_cast<UINT>(Mesh.Indices.size());
	SetNumIndices(NumIndices);

	DXLOG("Loaded model with %u vertices and %u indices\n", static_cast<UINT>(Mesh.Vertices.size()), NumIndices)

	VertexFactory.Init(Device, CommandList,
		VertexInitData<TypeOfVertex>::CreateVertexInitData(std::move(Mesh.Vertices), std::move(Mesh.Indices)));

	return hr;
}
