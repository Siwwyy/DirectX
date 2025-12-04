//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_TEXTURE_H_INCLUDED
#define D3D12_TEXTURE_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"

// Simple Smart Pointer following RAII idiom
template<typename PtrType>
struct SimpleSmartPointer
{
	DISABLE_COPY(SimpleSmartPointer)
	DISABLE_BASE_CTOR(SimpleSmartPointer)

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


// Texture class
class Texture
{

	DISABLE_COPY(Texture)

public:

	Texture()	= default;
	~Texture()	= default;

	SimpleSmartPointer<BYTE>&& Load(DXDevice * Device, DXGraphicsCommandList * CommandList, std::wstring TextureName = L"TextureBuffer");

	// Texture Handle
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(SIZE_T TextureIdx = 0);

	DXDescriptorHeap* GetDescriptorHeap()
	{
		return MainDescriptorHeap.Get();
	}

private:

	ComPtr<DXResource>			TextureBuffer; // the resource heap containing our texture
	ComPtr<DXDescriptorHeap>	MainDescriptorHeap;
	ComPtr<DXResource>			TextureBufferUploadHeap;
	UINT						IncrementDescriptorSize;
};

// Functions
int LoadImageDataFromFile(SimpleSmartPointer<BYTE>& ImageData, D3D12_RESOURCE_DESC& ResourceDescription, LPCWSTR Filename, int& BytesPerRow);

DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& WicFormatGUID);
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& WicFormatGUID);
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& DxgiFormat);

#endif // D3D12_TEXTURE_H_INCLUDED