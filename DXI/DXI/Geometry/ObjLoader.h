//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_OBJLOADER_H_INCLUDED
#define D3D12_OBJLOADER_H_INCLUDED

#include "pch.h"
#include "Vertex.h"

#include <string>

// LoadedMesh, the return type, lives in Vertex.h so every loader shares one.

// Reads a Wavefront OBJ.
//
// Handles the subset the format actually gets used for: v, vt, vn, and f with any of the
// v, v/vt, v//vn or v/vt/vn forms. Polygons with more than three corners are fanned into
// triangles. Negative (relative) indices are resolved. Everything else, materials and
// smoothing groups included, is skipped.
//
// Normals in the file are ignored and recomputed per face. That is not laziness: the frame
// solver writes a face's normal into each of its vertices by index, so a vertex shared
// between faces would end up holding whichever face was written last. The loader instead
// gives every triangle its own three vertices, which is what makes a flat shaded mesh come
// out right. The cost is a larger vertex buffer and an index buffer that is simply
// 0, 1, 2, ...
//
// The shaders interpolate the normal now, so smooth shading is available: it needs this
// loader to read the file's vn data, or to average face normals where the file has none.
//
// Throws std::runtime_error when the file cannot be opened or holds no triangles.
_NODISCARD LoadedMesh LoadObjFromFile(const std::wstring& Path);

#endif // D3D12_OBJLOADER_H_INCLUDED
