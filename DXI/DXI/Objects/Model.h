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
	~Model() override = default;

	// Reads the mesh off disk, then uploads it like any other primitive
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList) override;

	_NODISCARD const std::wstring& GetMeshPath() const noexcept
	{
		return MeshPath;
	}

private:

	std::wstring MeshPath;
};

#endif // D3D12_MODEL_H_INCLUDED
