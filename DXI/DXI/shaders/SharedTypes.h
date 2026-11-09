//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_SHARED_TYPES_H_INCLUDED
#define DXI_SHARED_TYPES_H_INCLUDED

// Definitions the CPU and the shaders both need, written once.
//
// Included by shaders/Common.hlsl, which every pass shader already includes, and by the
// C++ headers that build the matching data. DXC resolves includes relative to the file
// doing the including, which is why this sits beside Common.hlsl rather than in Utility.
//
// Written in HLSL spelling; the C++ branch below translates the type names. That way the
// definitions read as what the GPU sees and C++ is the one doing the adapting.
//
// WHAT THIS DOES NOT FIX
// Sharing a definition removes transcription mistakes, not layout mistakes. HLSL packs
// into 16 byte rows and will not straddle a vector across a row boundary; C++ packs
// tightly. The same text can therefore still mean two different layouts, so mind the note
// above the constant buffers below when adding or reordering a field.
//
// KEEP OUT OF HERE
//   bool        | four bytes in HLSL, one in C++, no macro saves this. Use uint.
//   enum class  | no HLSL equivalent. Numbers go here, the C++ enum derives from them.
//   matrices    | shareable, but HLSL cbuffers are column major and the upload transposes,
//                 which this does not change.

#ifdef __cplusplus

	#include <DirectXMath.h>
	#include <cstdint>

	// HLSL type names mean DirectXMath on this side. Undefined at the bottom of the file
	// so they cannot leak into anything that includes it.
	#define float2		DirectX::XMFLOAT2
	#define float3		DirectX::XMFLOAT3
	#define float4		DirectX::XMFLOAT4
	#define float4x4	DirectX::XMFLOAT4X4
	#define uint		uint32_t

	// alignas is a C++ spelling. An HLSL cbuffer gets its 256 byte placement from the API
	// and a plain struct needs no annotation at all.
	#define SHARED_ALIGN(N)		alignas(N)

	// A constant buffer is a plain struct on this side. Reg is deliberately unused: the
	// register token only means something to HLSL, and a macro parameter that never
	// appears in the replacement is not expanded, so CB_PER_* being undefined here is fine.
	#define CBUFFER_BEGIN(Name, Reg, Align)		struct SHARED_ALIGN(Align) Name {
	#define CBUFFER_END							};

#else

	#define SHARED_ALIGN(N)

	// In HLSL the fields become globals bound to a register. Align has no meaning here,
	// the API guarantees the placement.
	#define CBUFFER_BEGIN(Name, Reg, Align)		cbuffer Name : register(Reg) {
	#define CBUFFER_END							};

#endif // __cplusplus


// ---------------------------------------------------------------------------------------
//  Lights
// ---------------------------------------------------------------------------------------

// How many lights fit in the lights constant buffer
#define MAX_LIGHTS				4

// Value of ShadowSlot when a light casts no shadow
#define NO_SHADOW_CASTER		-1

// Kinds of light. LightType of Lights/LightBase.h derives its values from these.
#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_AREA			2

// A single light of the scene.
//
// Per type properties live here alongside the shared ones, tagged in the comment with which
// types read them. That is the only way a single array can carry them: every light sits in
// the same Lights[] and one shader loop walks the lot, so there is no compile time point at
// which "this one is a point light" is known. #ifdef cannot help, it resolves once for the
// whole shader. The cost of a field no type reads is four bytes per light.
//
// If per type data ever outgrows that, the next step is a pass per type, compiled once each
// with its own light struct and blended additively, which is where a real #ifdef would live.
//
// Laid out so no vector straddles a 16 byte row, which is what HLSL requires. The float3
// and float pairings below are deliberate: each fills exactly one row, which is why this
// needs no padding members. A float3 after a float would not, it would be pushed to the
// next row and silently disagree with the C++ side.
struct SHARED_ALIGN(16) Light
{
	float3	Position;		// World space position | Point, Area
	float	Intensity;		// Brightness multiplier

	float3	Direction;		// Direction the light points at | Directional, Area normal
	float	Range;			// Distance the light reaches | Point

	float3	Color;			// Linear colour
	uint	Type;			// One of LIGHT_TYPE_*

	float2	Size;			// Width and height of the emitting surface | Area
	int		ShadowSlot;		// Slice this light owns in the shadow atlas, NO_SHADOW_CASTER when it does not cast
	float	SourceRadius;	// How big the emitter is | Point, keeps the falloff finite at the centre

	float3	TypeParams;		// Unused | reserved for whichever type needs it next
	float	SpecularScale;	// Highlight strength of this light, 1 is neutral | all types
};


// ---------------------------------------------------------------------------------------
//  G-buffer
// ---------------------------------------------------------------------------------------

// Which target holds what. GBufferTarget of RenderTargets/GBuffer.h derives its values from
// these, the way LightType derives from LIGHT_TYPE_*, so the render target order, the SRV
// order in the light pass heap and the SV_Target order below cannot drift apart.
//
// The one thing that cannot be shared is the semantics themselves: HLSL will not build a
// SV_Target token out of a number, the same limitation that keeps ShaderRegisters.hlsl
// spelling out b0 and b1 by hand. GBufferOutput below is therefore written out in this
// order and has to be kept in it.
#define GBUFFER_ALBEDO				0
#define GBUFFER_NORMAL				1
#define GBUFFER_WORLD_POSITION		2
#define GBUFFER_MATERIAL_PARAMS		3
#define GBUFFER_TARGET_COUNT		4

// Depth is not one of the four. It is the depth buffer the base pass already writes, read
// back as a shader resource rather than copied into a target of its own, so it costs no
// extra memory. See DXIDepthBuffer, which carries a typeless resource with a D32_FLOAT view
// to render through and an R32_FLOAT view to sample.
#define GBUFFER_DEPTH_SRV_COUNT		1

// Output of BasePass, one entry per target. Mirrors the GBUFFER_* order above.
#ifdef BASE_PASS
struct GBufferOutput
{
	float4 Albedo			: SV_Target0;	// Surface colour out of the material diffuse slot
	float4 Normal			: SV_Target1;	// World space normal
	float4 WorldPosition	: SV_Target2;	// World space position, w marks that geometry was written here
	float4 MaterialParams	: SV_Target3;	// r metalness, g roughness, b ambient occlusion
};
#endif // BASE_PASS


// ---------------------------------------------------------------------------------------
//  Base pass surface flags and debug views
// ---------------------------------------------------------------------------------------

// Albedo alpha doubles as a flag for LightPass. A debug view has already decided what
// colour the pixel should be, so shading it again would hide the very thing being looked at.
#define SURFACE_LIT							1.0
#define SURFACE_UNLIT						0.0

// What BasePass writes into the albedo target instead of the surface colour.
// BasePassDebugMode of Passes/BasePass.h derives its values from these.
#define BASE_PASS_DEBUG_OFF					0
#define BASE_PASS_DEBUG_UV_DENSITY			1
#define BASE_PASS_DEBUG_UV_DDX				2
#define BASE_PASS_DEBUG_UV_DDY				3
#define BASE_PASS_DEBUG_MIP_LEVEL			4
#define BASE_PASS_DEBUG_WORLD_DERIVATIVES	5
#define BASE_PASS_DEBUG_NORMAL_DERIVATIVES	6
#define BASE_PASS_DEBUG_COUNT				7


// ---------------------------------------------------------------------------------------
//  Constant buffers
//
//  C++ always gets every struct. HLSL only gets the ones its pass actually binds, which is
//  not tidiness: b1 carries a different buffer in each pass, so declaring all of them at
//  once would put two cbuffers on one register. The guards mirror the root signatures.
//
//  Field names must also stay unique within a pass, because an HLSL cbuffer puts its
//  fields in global scope rather than behind the buffer name.
//
//  ADDING OR EDITING A BUFFER | start every vector on a 16 byte boundary
//
//  HLSL lays fields into 16 byte rows and will push a vector to the next row rather than
//  let it cross a boundary. C++ packs tightly and never does. So the same field order can
//  silently mean two different layouts, and the CPU then writes where the GPU is not
//  reading. Scalars are exempt and may sit at any offset.
//
//      float2  A;      // HLSL 0    C++ 0
//      float3  B;      // HLSL 16   C++ 8     <-- eight bytes adrift, no diagnostic
//
//  Two ways to keep them in step, in order of preference:
//
//    1. Pair a float3 with a scalar, or a float2 with two scalars, so each group fills a
//       row exactly. Light below does this four times over and needs no padding at all.
//    2. Where the order cannot change, add an explicit padding member so C++ leaves the
//       same gap HLSL would:
//
//           float2  A;
//           float   _PadA[2];   // HLSL skips these bytes, C++ has to be told to
//           float3  B;
//
//  A matrix always starts a row, so three matrices back to back are safe on their own.
//  Rounding the whole struct up with SHARED_ALIGN does not help here: it only sets the
//  start address and the total size, never the offsets in between.
// ---------------------------------------------------------------------------------------

// b0 | Per object. Rebound per draw, by both the base pass and every shadow view.
#if defined(__cplusplus) || defined(BASE_PASS) || defined(SHADOW_PASS)
CBUFFER_BEGIN(PrimitiveConstantBuffer, CB_PER_OBJECT, 256)
	float4x4 LocalToWorld;			// Only with translation
	float4x4 WorldToClip;			// Translation * View * Proj (Clip)

	// Normals can not use LocalToWorld. A non uniform scale shears a direction differently
	// than it moves a point, so a normal scaled that way stops being perpendicular to its
	// surface. The matrix that keeps it perpendicular is the inverse transpose of the
	// world matrix, which equals the inverse scale times the rotation.
	float4x4 LocalToWorldUniformScale;
CBUFFER_END
#endif

// b1 | Per view, light pass flavour. What the shading needs to know about the camera.
#if defined(__cplusplus) || defined(LIGHT_PASS)
CBUFFER_BEGIN(CameraConstantBuffer, CB_PER_VIEW, 256)
	float4   EyePosition;			// World space position of the camera
	float4x4 InvViewProj;			// Clip space back to world
CBUFFER_END
#endif

// b1 | Per view, shadow pass flavour. Root constants rather than a CBV, so 16 rather than
// 256: there can be eight views in a frame and rebinding a buffer for each would be waste.
#if defined(__cplusplus) || defined(SHADOW_PASS)
CBUFFER_BEGIN(ShadowViewConstants, CB_PER_VIEW, 16)
	float4x4 WorldToLightClip;		// World straight into this light's clip space
	float3   LightPosition;			// World space, a cube face measures distance from it
	float    LightRange;			// Distance is stored divided by this, so it fits 0 to 1
CBUFFER_END
#endif

// b1 | Per view, base pass flavour. A single value, so it rides in the root signature.
#if defined(__cplusplus) || defined(BASE_PASS)
CBUFFER_BEGIN(BasePassDebugConstants, CB_PER_VIEW, 4)
	uint DebugMode;					// One of BASE_PASS_DEBUG_*
CBUFFER_END
#endif

// b2 | Every light of the scene. Each one owns a slice of the shadow atlas; directional and
// area lights render a single depth view, so only those need a matrix here. A point light
// is looked up by direction, which is why its six face matrices never leave the shadow pass.
#if defined(__cplusplus) || defined(LIGHT_PASS)
CBUFFER_BEGIN(LightsConstantBuffer, CB_PER_LIGHTS, 256)
	Light    Lights[MAX_LIGHTS];				// All lights of the scene
	float4x4 ShadowViewProj[MAX_LIGHTS];		// World to that light's clip space | Directional, Area
	uint     NumLights;							// How many entries of Lights are filled
CBUFFER_END
#endif


#ifdef __cplusplus

	// The type aliases have done their job. Leaving float3 and friends defined would have
	// them rewrite unrelated code in any translation unit that includes this.
	#undef float2
	#undef float3
	#undef float4
	#undef float4x4
	#undef uint

#endif // __cplusplus

#endif // DXI_SHARED_TYPES_H_INCLUDED
