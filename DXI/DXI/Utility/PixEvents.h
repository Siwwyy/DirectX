//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_PIXEVENTS_H_INCLUDED
#define DXI_PIXEVENTS_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"

// PIX markers, written as a Begin/End pair with the work braced in between. They nest by call
// order, so every Begin needs its End on the same path out. Compiles to nothing without PIX.

#ifdef USE_PIX
	#include <pix3.h>
#endif

// Colour the event is drawn with in a capture, as 0xAARRGGBB with full alpha. Related work
// shares a hue so the timeline can be read without looking at the names.
namespace DXIPIXCOLORS
{
	constexpr UINT64 SHADOW_PASS	= 0xFF3E6FA8;	// blue, depth and distance views
	constexpr UINT64 BASE_PASS		= 0xFF4E8F4E;	// green, the G-buffer fill
	constexpr UINT64 LIGHT_PASS		= 0xFFC8903C;	// amber, the shading
	constexpr UINT64 TANGENT_FRAME	= 0xFFA0508F;	// plum, the tangent frame overlay
	constexpr UINT64 AXIS_GIZMO		= 0xFF3FA0A0;	// teal, the origin axes overlay
	constexpr UINT64 SELECTION		= 0xFFD08030;	// orange, the selection highlight
	constexpr UINT64 TRANSFORM_GIZMO = 0xFFC0C040;	// yellow, the move and rotate handles
	constexpr UINT64 FRAME_DUMP		= 0xFFB0A040;	// gold, copying buffers out for the EXR dump
	constexpr UINT64 UI				= 0xFF808080;	// grey, ImGui on top

	constexpr UINT64 VIEW			= 0xFF6E8FBF;	// one light, or one face of its cube
	constexpr UINT64 DRAW			= 0xFF9AA7B5;	// a single primitive
}

#ifdef USE_PIX

	#define DXI_PIX_BEGIN(CommandList, Color, ...)	PIXBeginEvent(CommandList, Color, __VA_ARGS__)
	#define DXI_PIX_END(CommandList)				PIXEndEvent(CommandList)

#else

	#define DXI_PIX_BEGIN(CommandList, Color, ...)	((void)0)
	#define DXI_PIX_END(CommandList)				((void)0)

#endif // USE_PIX

#endif // DXI_PIXEVENTS_H_INCLUDED
