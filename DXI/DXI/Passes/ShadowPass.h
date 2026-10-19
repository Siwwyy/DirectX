//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADOWPASS_H_INCLUDED
#define D3D12_SHADOWPASS_H_INCLUDED

#include "PassBase.h"

// What one shadow view needs on top of the per object matrices. Sent as root constants,
// because it changes per view and there can be eight views in a frame.
struct alignas(16) ShadowViewConstants
{
	DirectX::XMFLOAT4X4	WorldToLightClip;	// Already transposed for HLSL
	DirectX::XMFLOAT3	LightPosition;		// World space, a cube face measures distance from it
	float				LightRange;			// Distance is stored divided by this, so it fits 0 to 1
};

static_assert((sizeof(ShadowViewConstants) % 4) == 0, "Root constants are counted in 32 bit values");
constexpr UINT ShadowViewConstantCount = sizeof(ShadowViewConstants) / sizeof(UINT32);


// Renders the scene from a light.
//
// Two pipelines, because the two kinds of light store different things:
//   Depth    | Directional and area lights write depth only, with no pixel shader at all
//   Distance | A point light writes how far the light reached into a colour target, so a
//              lookup later is a single cube sample by direction
class ShadowPass : public PassBase
{
	DISABLE_COPY(ShadowPass)

public:

	// Ctors & DCtor
	ShadowPass();
	~ShadowPass() = default;

	// Init
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept override;

	// Render | Draws one primitive into whichever view is currently bound
	void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept override;

	// Pipeline that writes distance into a cube face, instead of depth only
	_NODISCARD FORCEINLINE DXPipelineState* GetDistancePipelineState() const noexcept
	{
		return DistancePSO.PipelineState.Get();
	}

	// Root parameter indices, the caller binds them before Render
	enum RootParamIdx : UINT
	{
		PrimitiveCBVIdx = 0,	// b0 | Per object, only LocalToWorld is read
		ShadowViewIdx			// b1 | One ShadowViewConstants, set as root constants
	};

private:

	// Writes the distance to the light rather than depth, for the cube faces
	GraphicsPSO		DistancePSO;
};

#endif // D3D12_SHADOWPASS_H_INCLUDED
