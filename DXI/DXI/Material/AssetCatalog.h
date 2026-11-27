//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_ASSETCATALOG_H_INCLUDED
#define D3D12_ASSETCATALOG_H_INCLUDED

#include "pch.h"
#include "LightBase.h"

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
	std::wstring	Folder;				// Absolute path of the Poly Haven folder, empty for a lone colour map
	std::wstring	DiffuseOnlyPath;
	int				LoadedIdx = -1;		// Index into the renderer's Materials once loaded, -1 until then
};

// How many primitives the scene can hold. The primitive constant buffer is created for
// this many, mapped once, and never resized, so it is the hard ceiling on spawning. Lives
// here rather than in the renderer because the panel greys out its button against it.
//
// Sized for a loaded scene rather than a handful of cubes: Sponza arrives as 103 primitives
// on its own, since glTF splits a mesh wherever the material changes. The cost of the
// ceiling is only address space in the constant buffer, 256 bytes per slot per frame, so
// it is set well clear of what the scene needs rather than exactly at it.
constexpr UINT MAX_PRIMITIVES = 256;

// Which built in shape a spawn uses, or a mesh out of the catalog
enum class SpawnMesh : UINT
{
	Cube,
	Plane,
	Sphere,
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
	UINT		Density		= 24;	// Latitude bands, when Mesh is Sphere

	// Set when the request is a drop on the view: where the cursor is, in the view's pixels. The object
	// is put where the ray under it meets the scene. Unset it goes in front of the camera.
	bool		bAtCursor	= false;
	int			CursorX		= 0;
	int			CursorY		= 0;
};

// A light the panel has asked for. No GPU upload involved, but it still waits for the
// frame so every scene change lands at the same point.
struct LightSpawnRequest
{
	LightType Type = LightType::Point;

	// As for a primitive: a drop puts the light above the point under the cursor, otherwise it is at the camera
	bool	bAtCursor	= false;
	int		CursorX		= 0;
	int		CursorY		= 0;
};

// A material the editor has asked an object to wear. Like a spawn it waits for the frame, since a
// material that is not loaded yet is loaded then, which records texture uploads.
struct MaterialAssignRequest
{
	int		PrimitiveIdx	= -1;	// The object, when bAtCursor is not set
	int		CatalogIdx		= -1;	// Into AssetCatalog::GetMaterials, -1 puts the default material back

	// Set for a drop on the view: the object under that pixel of the view gets it, and is selected
	bool	bAtCursor		= false;
	int		CursorX			= 0;
	int		CursorY			= 0;
};

// What the scene can be built out of, discovered by walking the content folders rather
// than listed in code. Scanned once at startup; the panel reads it to populate its
// pickers and the renderer loads an entry the first time something asks for it.
class AssetCatalog
{
public:

	AssetCatalog()	= default;
	~AssetCatalog()	= default;

	/**
	 * Walks the content root for material folders and the models folder for meshes
	 */
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
