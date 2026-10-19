//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADOWATLAS_H_INCLUDED
#define D3D12_SHADOWATLAS_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "LightTypes/LightBase.h"

// Shadow storage for every light of the scene, kept as texture arrays rather than tiles
// of one big surface. Slices cost nothing to address and cannot bleed into each other,
// which a tiled atlas has to guard against at every edge.
//
// Two of them, because the two kinds of light ask different questions:
//   Depth2D   | Directional and area lights render one view, so a depth slice each and
//               the lookup is a projection plus a depth compare
//   DistanceCube | A point light shines everywhere, so it renders six faces into a cube
//               and stores the distance to the light rather than a depth. The lookup is
//               then a single sample by direction, with no face maths in the shader.
class ShadowAtlas
{
	DISABLE_COPY_MOVE(ShadowAtlas)

public:

	// Faces of a cube, the point light renders one view per face
	static constexpr UINT CUBE_FACE_COUNT = 6;

	ShadowAtlas()	= default;
	~ShadowAtlas()	= default;

	// Creates both arrays with room for MAX_LIGHTS casters of either kind
	void Initialize(DXDevice * Device, const UINT Resolution, const UINT CubeResolution);

	// Formats the passes need for their PSOs
	_NODISCARD static DXGI_FORMAT GetDepthFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	}
	_NODISCARD static DXGI_FORMAT GetDepthSrvFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;	// a depth format can not be read directly
	}
	_NODISCARD static DXGI_FORMAT GetDistanceFormat() noexcept
	{
		return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;	// one channel, the distance to the light
	}

	// Writes both views, the 2D array first and the cube array after it
	void CreateShaderResourceViews(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE StartHandle, const UINT IncrementSize) const;

	// Barriers | The passes write these, LightPass samples them
	void TransitionToWrite(DXGraphicsCommandList * CommandList);
	void TransitionToShaderResource(DXGraphicsCommandList * CommandList);

	// Binds one depth slice for a directional or area light and resets it
	void BindDepthSlice(DXGraphicsCommandList * CommandList, const UINT Slot) const;

	// Binds one cube face of one point light. The colour target takes the distance, the
	// shared depth is only there to resolve what is in front of what.
	void BindCubeFace(DXGraphicsCommandList * CommandList, const UINT Slot, const UINT FaceIdx) const;

	// Getters
	_NODISCARD FORCEINLINE const CD3DX12_VIEWPORT& GetViewPort() const noexcept
	{
		return ViewPort;
	}
	_NODISCARD FORCEINLINE const CD3DX12_RECT& GetScissorRect() const noexcept
	{
		return ScissorRect;
	}
	_NODISCARD FORCEINLINE const CD3DX12_VIEWPORT& GetCubeViewPort() const noexcept
	{
		return CubeViewPort;
	}
	_NODISCARD FORCEINLINE const CD3DX12_RECT& GetCubeScissorRect() const noexcept
	{
		return CubeScissorRect;
	}

	// How many descriptors CreateShaderResourceViews writes
	_NODISCARD static constexpr UINT GetSrvCount() noexcept
	{
		return 2;
	}

private:

	// Value the depth slices are cleared to, 1 is the far plane
	static constexpr float DEPTH_CLEAR_VALUE = 1.0f;

	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetDepthDsvHandle(const UINT Slot) const;
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCubeRtvHandle(const UINT Slot, const UINT FaceIdx) const;

	// Directional and area lights, one depth slice each
	ComPtr<DXResource>			DepthArray;
	ComPtr<DXDescriptorHeap>	DepthDsvHeap;

	// Point lights, six distance faces each plus one shared depth to sort them
	ComPtr<DXResource>			DistanceCubeArray;
	ComPtr<DXDescriptorHeap>	CubeRtvHeap;
	ComPtr<DXResource>			CubeDepth;
	ComPtr<DXDescriptorHeap>	CubeDsvHeap;

	UINT						DsvIncrementSize	= 0;
	UINT						RtvIncrementSize	= 0;
	UINT						Resolution			= 0;
	UINT						CubeResolution		= 0;

	CD3DX12_VIEWPORT			ViewPort			= CD3DX12_VIEWPORT(0.f, 0.f, 0.f, 0.f);
	CD3DX12_RECT				ScissorRect			= CD3DX12_RECT(0, 0, 0, 0);
	CD3DX12_VIEWPORT			CubeViewPort		= CD3DX12_VIEWPORT(0.f, 0.f, 0.f, 0.f);
	CD3DX12_RECT				CubeScissorRect		= CD3DX12_RECT(0, 0, 0, 0);

	// State both arrays currently sit in, the transitions are no-ops when already there
	bool						bWritable			= false;
};

#endif // D3D12_SHADOWATLAS_H_INCLUDED
