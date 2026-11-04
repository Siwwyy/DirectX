//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_TEXTURE_H_INCLUDED
#define D3D12_TEXTURE_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"

// Simple Smart Pointer following RAII idiom, using malloc!
// No ctor for allocated memory, only raw buffers allowed
template<typename PtrType>
struct SimpleSmartPointer
{
	// Checking if type is POD
	static_assert(std::is_trivially_copyable_v<PtrType>, "SimpleSmartPointer works only for POD like types e.g., int, float, BYTE etc.");

	// Disable unnecessary methods
	DISABLE_COPY(SimpleSmartPointer)
	DISABLE_DEFAULT_CTOR(SimpleSmartPointer)

	// Ctors
	SimpleSmartPointer(const unsigned int size)
		:	ptr(static_cast<PtrType*>(malloc(size)))
		,	size(size)
	{ }

	SimpleSmartPointer(std::nullptr_t null_ptr)
		:	ptr(null_ptr)
		,	size(0)
	{ }

	SimpleSmartPointer(SimpleSmartPointer&& SimpleSmartPointer)
		: ptr(SimpleSmartPointer.ptr)
		, size(SimpleSmartPointer.size)
	{
		SimpleSmartPointer.Reset();
	}

	// Dctor
	~SimpleSmartPointer()
	{
		DeleteUnderlayingPtr();
	}

	// Move assignment operator
	SimpleSmartPointer& operator=(SimpleSmartPointer&& SimpleSmartPointer) noexcept
	{
		// If we want to move data from the same object, lets skip it
		if (this == &SimpleSmartPointer)
		{
			return *this;
		}

		// Remove old data
		if (ptr)
		{
			free(ptr);
		}

		// Assign new data
		ptr		= std::exchange(SimpleSmartPointer.ptr, nullptr);
		size	= SimpleSmartPointer.size;
		SimpleSmartPointer.Reset();
		return *this;
	}

	// Reset
	void Reset()
	{
		ptr = nullptr;
		size = 0;
	}

	// Release
	PtrType* Release() 
	{
		return std::exchange(ptr, nullptr);
	}

	// Get underlaying pointer
	_NODISCARD FORCEINLINE PtrType* const Get()
	{
		return ptr;
	}

	// Get size of already allocated data
	_NODISCARD FORCEINLINE unsigned int GetSize() const
	{
		return size;
	}

private:

	/* Helper function to invoke delete on the underlying raw pointer */
	void DeleteUnderlayingPtr()
	{
		if (ptr != nullptr)
		{
			free(ptr);
			ptr = nullptr;
		}
	}

	// Variables
	PtrType * ptr;
	unsigned int size;

};

template<typename PtrType>
SimpleSmartPointer<PtrType> make_simple_smart_pointer(const unsigned int size)
{
	return SimpleSmartPointer<PtrType>(size);
}


// Functions | Image decoding, the GPU side lives in Material
int LoadImageDataFromFile(SimpleSmartPointer<BYTE>& ImageData, D3D12_RESOURCE_DESC& ResourceDescription, LPCWSTR Filename, int& BytesPerRow);

// Box filters an image down to a full mip chain, entry 0 being the original and each one
// after it half the size, stopping at 1x1.
//
// The texel size comes from the format, it is not always four. A grayscale png decodes to
// DXGI_FORMAT_R8_UNORM and carries a single byte per texel, which is what the roughness,
// metalness and occlusion maps are.
//
// Averaging works on bytes, so only formats with 8 bit channels can be filtered this way.
// Anything else comes back as a single mip rather than as garbage.
//
// Without these a texture is sampled at full resolution no matter how few pixels it
// covers, which is what makes a surface shimmer as it moves into the distance.
_NODISCARD std::vector<std::vector<BYTE>> GenerateMipChain(const BYTE * Source, const UINT Width, const UINT Height, const UINT RowPitch, const DXGI_FORMAT Format);

// Whether GenerateMipChain can actually filter this format, rather than just copy mip 0
_NODISCARD bool CanGenerateMips(const DXGI_FORMAT Format);

// Bytes a single texel occupies, which is the stride between two neighbours in a row.
// Returns 0 for a format this does not know, rather than the -1 the bits per pixel
// helper hands back, so callers can test it without the sign trap.
_NODISCARD UINT GetFormatBytesPerTexel(const DXGI_FORMAT Format);

// How many mips an image of this size ends up with
_NODISCARD UINT GetMipCount(const UINT Width, const UINT Height);

DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& WicFormatGUID);
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& WicFormatGUID);
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& DxgiFormat);

#endif // D3D12_TEXTURE_H_INCLUDED