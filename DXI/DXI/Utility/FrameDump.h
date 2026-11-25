//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_FRAMEDUMP_H_INCLUDED
#define DXI_FRAMEDUMP_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIDevice.h"
#include "DXISwapChain.h"
#include "DXIDepthBuffer.h"
#include "DXITextureReadback.h"
#include "GBuffer.h"

#include <string>

// Writes every buffer of the pipeline for a single frame out as OpenEXR files, so they can be
// opened in Nuke, Blender, DJV, RenderDoc's image viewer or read from Python with the OpenEXR package.
//
// It takes two steps with a GPU wait between them, because a texture can only be copied while a
// frame is being recorded, and only read once the GPU has run the copy:
//
//      Dump.Capture(...);          // inside the frame, after the pass whose output is wanted
//      ...frame is submitted, the caller waits for its fence...
//      Dump.Write(RootDir);        // the data is there now
//
// One folder per dump, named after the time, one file per buffer:
//
//      color.exr           R G B A         the frame as presented, display referred, 0 to 1
//      depth.exr           Z ZDevice       Z is the distance along the view axis in world units,
//                                          ZDevice is what the depth buffer holds, 0 near to 1 far
//      velocity.exr        R G             motion in pixels since the previous frame, current minus
//                                          previous position, x right and y down the screen
//      roughness.exr       Y               one channel, straight out of the material params
//      material_params.exr R G B           metalness, roughness, ambient occlusion
//      albedo.exr          R G B A
//      normal.exr          R G B           world space, signed
//      world_position.exr  R G B A         world space, A is 1 where geometry was written
//
// Everything is stored as 32 bit float, uncompressed. That makes them large, a 1920x1080 frame
// is around 33 MB for the position buffer alone, but exact and fast to write.
class FrameDump
{
public:

	struct Result
	{
		bool			bSuccess	= false;
		UINT			FileCount	= 0;	// how many files made it to disk
		std::wstring	Directory;			// where they went
		std::wstring	Message;			// what to show the user, says what failed when something did
	};

	FrameDump()		= default;
	~FrameDump()	= default;

	/**
	 * Records the copy of every buffer into readback memory. Call it when the buffers hold what
	 * is wanted, which is right after the light pass: colour is shaded but the overlays and the
	 * UI are not on it yet.
	 *
	 * Every buffer is returned to the state it was found in.
	 *
	 * @param BackBufferState   State the back buffer is in at this point
	 * @param ProjA, ProjB      _33 and _43 of the projection, to turn depth back into a distance
	 */
	void Capture(const DXIDevice& Device,
				 DXGraphicsCommandList * CommandList,
				 const DXISwapChain& SwapChain,
				 const UINT BackBufferIdx,
				 const D3D12_RESOURCE_STATES BackBufferState,
				 const GBuffer& SceneGBuffer,
				 const DXIDepthBuffer& DepthBuffer,
				 const float ProjA,
				 const float ProjB);

	/**
	 * Reads the copies back and writes the files into a new, time stamped folder inside RootDir.
	 * Only call once the GPU has finished the frame Capture was recorded into.
	 */
	_NODISCARD Result Write(const std::wstring& RootDir);

	// True from Capture until Write
	_NODISCARD FORCEINLINE bool IsPending() const noexcept
	{
		return bPending;
	}

private:

	DXITextureReadback	ColorReadback;
	DXITextureReadback	DepthReadback;
	DXITextureReadback	GBufferReadbacks[GBufferTargetCount];

	float				DepthProjA	= 0.f;
	float				DepthProjB	= 0.f;
	bool				bPending	= false;
};

#endif // DXI_FRAMEDUMP_H_INCLUDED
