//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_OBJLOADER_H_INCLUDED
#define D3D12_OBJLOADER_H_INCLUDED

#include "pch.h"
#include "Vertex.h"

#include <string>

// LoadedMesh, the return type, lives in Vertex.h so every loader shares one.

/**
 * Reads a Wavefront OBJ: v, vt, vn and f in any of its index forms, polygons fanned into
 * triangles, everything else skipped. The file's normals are ignored and recomputed per face,
 * because the frame solver writes by index and a shared vertex would keep only the last face
 * written, so every triangle gets its own three vertices and the indices are just 0, 1, 2...
 *
 * Throws std::runtime_error when the file cannot be opened or holds no triangles.
 */
_NODISCARD LoadedMesh LoadObjFromFile(const std::wstring& Path);

#endif // D3D12_OBJLOADER_H_INCLUDED
