//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_PIXEVENTS_H_INCLUDED
#define DXI_PIXEVENTS_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"

// PIX markers, so a capture reads as the structure of the frame rather than a flat run of
// draws. Written as an explicit pair with the work braced in between, which makes the
// extent of the event obvious at a glance:
//
//     DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::BASE_PASS, L"Base Pass | %zu primitives", Count);
//     {
//         ...
//     }
//     DXI_PIX_END(CommandList);
//
// PIX nests by call order, so a Begin inside another Begin is what produces the tree in a
// capture. Every Begin needs its End on the same path out.
//
// The format string is handed to PIX untouched, so the printf style conversions it supports
// all work. Prefer putting identifying detail in the name: a capture full of events called
// "Draw" is barely better than no events at all.
//
// Compiles to nothing when PIX is unavailable. CMake defines USE_PIX only when it found
// both the header and the import library, which is also why pix3.h is included here rather
// than in each pass.

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
