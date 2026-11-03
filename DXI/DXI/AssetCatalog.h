//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_ASSETCATALOG_H_INCLUDED
#define D3D12_ASSETCATALOG_H_INCLUDED

#include "pch.h"
#include "LightTypes/LightBase.h"

#include <string>

// A mesh found on disk, ready to be spawned
struct ModelAsset
{
	std::wstring Name;			// Folder name, which is what the panel shows
	std::wstring MeshPath;		// Absolute path to the .obj
};

// A material found on disk. Two shapes exist, told apart by DiffuseOnlyPath:
//   empty     | a Poly Haven style folder, every map named <folder>_<suffix>_1k.png
//   non empty | a single colour map shipped beside a model, the rest taken as neutral
struct MaterialAsset
{
	std::wstring	Name;
	std::wstring	DiffuseOnlyPath;
	int				LoadedIdx = -1;		// Index into the renderer's Materials once loaded, -1 until then
};

// How many primitives the scene can hold. The primitive constant buffer is created for
// this many, mapped once, and never resized, so it is the hard ceiling on spawning. Lives
// here rather than in the renderer because the panel greys out its button against it.
constexpr UINT MAX_PRIMITIVES = 32;

// Which built in shape a spawn uses, or a mesh out of the catalog
enum class SpawnMesh : UINT
{
	Cube,
	Plane,
	Model
};

// A primitive the panel has asked for. Collected while the UI is drawn and drained by the
// renderer on its next frame, because spawning records copy commands and the command list
// is only open once the frame has begun.
struct PrimitiveSpawnRequest
{
	SpawnMesh	Mesh		= SpawnMesh::Cube;
	int			ModelIdx	= -1;	// Into AssetCatalog::GetModels, when Mesh is Model
	int			MaterialIdx	= -1;	// Into AssetCatalog::GetMaterials, -1 keeps the default material
};

// A light the panel has asked for. No GPU upload involved, but it still waits for the
// frame so every scene change lands at the same point.
struct LightSpawnRequest
{
	LightType Type = LightType::Point;
};

// What the scene can be built out of, discovered by walking the content folders rather
// than listed in code. Scanned once at startup; the panel reads it to populate its
// pickers and the renderer loads an entry the first time something asks for it.
class AssetCatalog
{
public:

	AssetCatalog()	= default;
	~AssetCatalog()	= default;

	// Walks the content root for material folders and the models folder for meshes
	void Scan();

	_NODISCARD const std::vector<ModelAsset>& GetModels() const noexcept
	{
		return Models;
	}

	_NODISCARD const std::vector<MaterialAsset>& GetMaterials() const noexcept
	{
		return Materials;
	}

	// Non const, because the renderer writes LoadedIdx back once it has loaded one
	_NODISCARD std::vector<MaterialAsset>& GetMaterials() noexcept
	{
		return Materials;
	}

private:

	std::vector<ModelAsset>		Models;
	std::vector<MaterialAsset>	Materials;
};

#endif // D3D12_ASSETCATALOG_H_INCLUDED
