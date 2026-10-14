//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_CUBEMAP_H_INCLUDED
#define D3D12_CUBEMAP_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Texture.h"

// Cube map built out of an equirectangular panorama.
// Free sky assets ship as a single 2:1 panorama rather than six faces, so the faces are
// projected out of it on the CPU at load time and uploaded as one cube texture.
//
// Two flavours come out of the same panorama:
//   Radiance   | what the sky looks like, mipped so roughness can blur the reflection
//   Irradiance | the sky cosine convolved over the hemisphere, for the diffuse reflection
class CubeMap
{
	DISABLE_COPY_MOVE(CubeMap)

public:

	// A cube always has six of them, in the D3D order +X -X +Y -Y +Z -Z
	static constexpr UINT FACE_COUNT = 6;

	CubeMap()	= default;
	~CubeMap()	= default;

	// Sky as it looks. Builds a full mip chain, every smaller mip is a blurrier version
	// of the sky, which is what makes a rough surface reflect a soft blob.
	void LoadRadiance(DXDevice * Device,
						DXGraphicsCommandList * CommandList,
						const std::wstring& PanoramaPath,
						const UINT FaceResolution = 512,
						const std::wstring& Name = L"Radiance CubeMap");

	// Sky gathered over the hemisphere around every direction, which is the light a
	// diffuse surface receives. Low resolution on purpose, irradiance barely varies.
	void LoadIrradiance(DXDevice * Device,
						DXGraphicsCommandList * CommandList,
						const std::wstring& PanoramaPath,
						const UINT FaceResolution = 32,
						const std::wstring& Name = L"Irradiance CubeMap");

	// Writes the cube shader resource view at the given slot. The caller owns the heap,
	// because the pass that samples this also samples other things out of one heap.
	void CreateShaderResourceView(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle) const;

	// Getters
	_NODISCARD FORCEINLINE bool IsValid() const noexcept
	{
		return CubeBuffer != nullptr;
	}
	_NODISCARD FORCEINLINE UINT GetFaceResolution() const noexcept
	{
		return FaceResolution;
	}
	_NODISCARD FORCEINLINE UINT GetMipCount() const noexcept
	{
		return MipCount;
	}

	// Format the faces are stored in
	_NODISCARD static DXGI_FORMAT GetFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	}

private:

	// Faces of one mip, laid out face after face
	using FaceData = std::vector<BYTE>;

	// Creates the cube resource out of already projected faces and uploads every mip
	void Upload(DXDevice * Device,
				DXGraphicsCommandList * CommandList,
				const std::vector<FaceData>& Mips,
				const std::wstring& Name);

	ComPtr<DXResource>	CubeBuffer;				// the cube texture, six array slices
	ComPtr<DXResource>	CubeBufferUploadHeap;	// staging the faces are copied from
	UINT				FaceResolution	= 0;	// square, so one number covers both axes
	UINT				MipCount		= 1;	// how many mips the chain ended up with
};

#endif // D3D12_CUBEMAP_H_INCLUDED
