//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_PICKING_H_INCLUDED
#define DXI_PICKING_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "Primitive.h"

#include <memory>
#include <vector>

// What a mouse click is turned into to find the object under it: a half line from the camera
// through the pixel, tested against the objects' triangles on the CPU. The GPU already knows
// what is under every pixel, but it can not be asked cheaply, and a click is once in a while.
struct PickRay
{
	DirectX::XMFLOAT3	Origin		= { 0.f, 0.f, 0.f };	// A point on the near plane, under the cursor
	DirectX::XMFLOAT3	Direction	= { 0.f, 0.f, 1.f };	// Unit length, into the scene
};

/**
 * The ray under a pixel.
 *
 * @param CursorPixels  Position in the viewport, the origin at its top left corner like the cursor reports it
 * @param ViewportSize  Size of the viewport in pixels
 * @param View, Proj    The camera that drew the picture the cursor is looking at
 */
_NODISCARD PickRay BuildPickRay(const DirectX::XMFLOAT2& CursorPixels,
								const DirectX::XMFLOAT2& ViewportSize,
								DirectX::FXMMATRIX View,
								DirectX::CXMMATRIX Proj);

/**
 * Where a ray first meets a primitive's surface, if it does. Both sides of a triangle count,
 * so an open mesh like the plane can be picked from behind.
 *
 * @param OutDistance   Along the ray, in world units, only written on a hit
 */
_NODISCARD bool RayIntersectsPrimitive(const PickRay& Ray, const Primitive& Target, float& OutDistance);

/**
 * The primitive the ray meets first.
 *
 * @return Its index, or -1 when the ray hits nothing
 */
_NODISCARD int PickPrimitive(const PickRay& Ray, const std::vector<std::unique_ptr<Primitive>>& Primitives);

#endif // DXI_PICKING_H_INCLUDED
