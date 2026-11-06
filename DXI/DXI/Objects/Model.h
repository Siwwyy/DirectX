//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_MODEL_H_INCLUDED
#define D3D12_MODEL_H_INCLUDED

#include "Primitive.h"

// A primitive whose geometry comes from a file rather than a hard coded array.
//
// Cube and Plane build their vertices at static initialization, so their buffers are sized
// at compile time. A model cannot be: the file has to be read first. The path is taken at
// construction and the mesh is only loaded in Init, which is where a device and a command
// list exist to upload it with.
class Model : public Primitive
{
	DISABLE_COPY(Model)

public:

	Model() = default;

	// MeshPath is absolute, build it with DXIPATHS::GetModelDir
	Model(std::wstring Name, std::wstring MeshPath);

	// Geometry that has already been parsed.
	//
	// A Wavefront file holds one mesh, so pointing a Model at a path and letting it read
	// its own works. A glTF file holds many, split wherever the material changes, and
	// reparsing the file once per piece would be absurd. The scene loader parses once and
	// hands each piece over here instead.
	Model(std::wstring Name, LoadedMesh Mesh);

	~Model() override = default;

	// Uploads the mesh, reading it off disk first if it was not supplied at construction
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList) override;

	_NODISCARD const std::wstring& GetMeshPath() const noexcept
	{
		return MeshPath;
	}

private:

	std::wstring	MeshPath;

	// Set when the geometry arrived pre-parsed. Cleared in Init once it has been uploaded,
	// because past that point the GPU copy is the only one that matters and a scene the
	// size of Sponza would otherwise keep a second one in system memory for nothing.
	LoadedMesh		PreloadedMesh;
};

#endif // D3D12_MODEL_H_INCLUDED
