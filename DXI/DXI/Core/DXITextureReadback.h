//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_TEXTUREREADBACK_H_INCLUDED
#define DXI_TEXTUREREADBACK_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"

#include <vector>

// Copies a texture into memory the CPU can read.
//
// A GPU texture can not be read from the CPU, it lives in a default heap in whatever layout
// the hardware likes. The way out is a copy into a buffer in a readback heap, which the CPU can
// map, and that copy is a command like any other: it only happens when the list it was
// recorded on has run. So this is two steps with a wait in between, and the wait belongs to
// the caller:
//
//      Readback.RecordCopy(...);       // while recording the frame
//      ...submit the list, wait for the GPU...
//      Readback.Read(Pixels);          // now the data is there
class DXITextureReadback
{
public:

	DXITextureReadback()	= default;
	~DXITextureReadback()	= default;

	/**
	 * Records copying the first mip of a texture into a readback buffer made for it.
	 *
	 * The texture is moved to the copy source state, copied, and moved back, so it ends in
	 * the state it started in and nothing around this call has to know. The caller says what
	 * that state is, a texture does not know it itself.
	 *
	 * @param CopyFormat    How the pixels are read. A format of the same family as the texture's,
	 *                      which matters for depth: a typeless depth texture is read as R32_FLOAT.
	 * @param SourceState   State the texture is in right now, and will be left in.
	 */
	void RecordCopy(DXDevice * Device,
					DXGraphicsCommandList * CommandList,
					DXResource * Source,
					const DXGI_FORMAT CopyFormat,
					const D3D12_RESOURCE_STATES SourceState);

	/**
	 * Decodes what the copy brought in to floats, ChannelCount of them per pixel, row by row
	 * from the top. Only valid once the GPU has finished the list RecordCopy went into.
	 *
	 * 8 bit unorm channels come out in 0 to 1, half floats and floats as they are.
	 *
	 * @return False when nothing was recorded, or the format is one this does not decode.
	 */
	_NODISCARD bool Read(std::vector<float>& OutPixels) const;

	// Getters, valid after RecordCopy
	_NODISCARD FORCEINLINE bool IsRecorded() const noexcept		{ return Buffer != nullptr; }
	_NODISCARD FORCEINLINE UINT GetWidth() const noexcept		{ return Width; }
	_NODISCARD FORCEINLINE UINT GetHeight() const noexcept		{ return Height; }
	_NODISCARD UINT GetChannelCount() const noexcept;

	/**
	 * Channels of a format this knows how to decode, 0 for any other
	 */
	_NODISCARD static UINT GetChannelCount(const DXGI_FORMAT Format) noexcept;

private:

	ComPtr<DXResource>					Buffer;		// readback heap buffer the copy lands in
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	Layout		= {};	// where the rows sit in the buffer, and how far apart
	UINT64								BufferBytes	= 0;	// size of the buffer, the last row is not padded out to the pitch
	UINT								Width		= 0;
	UINT								Height		= 0;
	DXGI_FORMAT							Format		= DXGI_FORMAT_UNKNOWN;
};

#endif // DXI_TEXTUREREADBACK_H_INCLUDED
