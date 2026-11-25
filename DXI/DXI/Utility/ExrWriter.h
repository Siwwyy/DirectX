//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_EXRWRITER_H_INCLUDED
#define DXI_EXRWRITER_H_INCLUDED

#include <cstdint>
#include <string>
#include <vector>

// Writes OpenEXR files, and only that: scanline, uncompressed, 32 bit float channels.
//
// Nothing else is needed for dumping render targets, and the format is simple enough that
// writing it is shorter than fetching a library and keeping it in step with the build. The
// price is size, an uncompressed 1080p RGBA float image is 33 MB. Anything that reads EXR
// opens these: Nuke, Blender, Photoshop, OpenImageIO, the OpenEXR package.
//
// Deliberately no headers of the engine in here, so this builds and tests on its own.
namespace DXIEXR
{
	// One named plane of an image. Pixels run row by row, top row first, which is how a
	// texture read back from the GPU is laid out, and how EXR stores them.
	struct Channel
	{
		std::string			Name;		// "R", "G", "B", "A", "Z", "Y", or any other name
		std::vector<float>	Pixels;		// Width * Height values
	};

	/**
	 * Writes one image to a file.
	 *
	 * Channels may come in any order, EXR wants them sorted by name and this sorts them. Every
	 * channel needs exactly Width * Height pixels.
	 *
	 * @param OutError  Filled with the reason when this returns false. May be null.
	 * @return          True when the whole file was written.
	 */
	bool WriteFile(const std::wstring& Path,
					const uint32_t Width,
					const uint32_t Height,
					const std::vector<Channel>& Channels,
					std::string * const OutError = nullptr);
}

#endif // DXI_EXRWRITER_H_INCLUDED
