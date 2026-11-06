//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_GLTFLOADER_H_INCLUDED
#define D3D12_GLTFLOADER_H_INCLUDED

#include "pch.h"
#include "Vertex.h"

#include <string>

// One drawable piece of a glTF file.
//
// glTF splits a mesh wherever the material changes, and calls each piece a primitive. That
// is the same granularity our Primitive draws at, one vertex buffer bound to one material,
// so a primitive here maps to exactly one Model in the scene. Sponza arrives as 103 of
// them over 25 materials.
struct GltfPrimitive
{
	LoadedMesh		Mesh;
	std::wstring	Name;					// Mesh name plus the primitive's position in it, for the panel and for PIX
	int				MaterialIdx = -1;		// Into GltfScene::Materials, -1 when the primitive asks for no material
};

// A glTF material, cut down to what our Material can actually consume.
//
// glTF describes a metallic-roughness surface, which is close to our slots but not the
// same shape: roughness and metalness arrive packed into the green and blue channels of
// one image rather than as two files. The path is reported as it stands and the caller
// decides what to do about the packing.
struct GltfMaterial
{
	std::wstring	Name;
	std::wstring	BaseColorPath;				// Diffuse slot
	std::wstring	NormalPath;					// Normal slot
	std::wstring	MetallicRoughnessPath;		// G is roughness, B is metalness
	bool			bDoubleSided	= false;	// Drives the two sided PSO
	bool			bAlphaMask		= false;	// alphaMode MASK, which nothing honours yet
};

// Everything one file contributes to the scene, already flattened.
struct GltfScene
{
	std::vector<GltfPrimitive>	Primitives;
	std::vector<GltfMaterial>	Materials;
};

// Reads a glTF 2.0 file, either .gltf with a sidecar .bin or self contained .glb.
//
// Parsing is cgltf's job. What this adds is the three conversions that stand between a
// glTF file and our vertex buffer:
//
// Hierarchy. glTF puts meshes on nodes in a tree and every node carries a transform, so a
// mesh's vertices are only meaningful once the chain of parents above it has been applied.
// The tree is walked here and each primitive comes back already in world space, since
// nothing downstream has anywhere to keep a parent in.
//
// Handedness. glTF is right handed with +Y up, our projection and view matrices are left
// handed. The scene is mirrored on Z to cross over, which also reverses how every triangle
// winds and flips the sign the bitangent needs, both of which are corrected here.
//
// Attributes. glTF supplies positions, normals, tangents and uvs separately and any of the
// last three may be missing. Whatever is absent is generated from the triangles, and the
// bitangent our layout stores is built from the normal and the tangent's handedness.
//
// Throws std::runtime_error when the file cannot be parsed, its buffers cannot be found or
// it holds no triangles.
_NODISCARD GltfScene LoadGltfFromFile(const std::wstring& Path);

#endif // D3D12_GLTFLOADER_H_INCLUDED
