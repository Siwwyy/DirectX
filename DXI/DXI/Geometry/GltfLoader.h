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

/**
 * Reads a glTF 2.0 file, either .gltf with a sidecar .bin or self contained .glb. cgltf does
 * the parsing, this adds the three conversions between a glTF file and our vertex buffer:
 * the node tree is flattened so every primitive comes back in world space, the scene is
 * mirrored on Z from glTF's right handed space with the winding and bitangent sign corrected
 * to match, and any missing normals, tangents or uvs are generated from the triangles.
 *
 * Throws std::runtime_error when the file cannot be parsed, its buffers cannot be found or
 * it holds no triangles.
 */
_NODISCARD GltfScene LoadGltfFromFile(const std::wstring& Path);

#endif // D3D12_GLTFLOADER_H_INCLUDED
