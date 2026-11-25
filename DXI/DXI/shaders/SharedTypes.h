//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_SHARED_TYPES_H_INCLUDED
#define DXI_SHARED_TYPES_H_INCLUDED

// Definitions the CPU and the shaders both need, written once. Included by Common.hlsl, which
// sits beside it because DXC resolves includes relative to the including file, and by the C++
// headers that build the matching data. Written in HLSL spelling, C++ does the adapting.
//
// This removes transcription mistakes, not layout mistakes, so mind the note above the
// constant buffers below when adding or reordering a field.
//
// KEEP OUT OF HERE
//   bool        | four bytes in HLSL, one in C++, no macro saves this. Use uint.
//   enum class  | no HLSL equivalent. Numbers go here, the C++ enum derives from them.
//   matrices    | shareable, but HLSL cbuffers are column major and the upload transposes.

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

	// A plain struct here. No alignas: 256 belongs to the view, see AlignConstantBufferSize
	#define CBUFFER_BEGIN(Name, Reg)	struct Name {
	#define CBUFFER_END					};

#else

	// Fields become globals bound to a register, the API handles the placement
	#define CBUFFER_BEGIN(Name, Reg)	cbuffer Name : register(Reg) {
	#define CBUFFER_END					};

#endif // __cplusplus


// ---------------------------------------------------------------------------------------
//  Lights
// ---------------------------------------------------------------------------------------

// How many lights fit in the lights constant buffer
#define MAX_LIGHTS				8

// Value of ShadowSlot when a light casts no shadow
#define NO_SHADOW_CASTER		-1

// Kinds of light. LightType of Lights/LightBase.h derives its values from these.
#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_AREA			2
#define LIGHT_TYPE_SPOT			3

// Where the sun sits. D3D12Renderer::AddLight puts the directional light first, so a shader
// reaches it without searching. A scene can have none, so check Type before using it
#define SUN_LIGHT_INDEX			0

// A single light of the scene. Per type properties sit alongside the shared ones, tagged with
// which types read them, because one loop walks a single Lights[] and no #ifdef can know which
// type a given element is. The float3 and float pairings below each fill one 16 byte row
// exactly, which is why this needs no padding.
// w follows the homogeneous rule, so either vector can go through a matrix unchanged: a
// position carries w 1 and picks up translation, a direction carries w 0 and does not.
// A directional light has no position, so its Position.w is 0 as well.
struct Light
{
	float4	Position;		// World space position, w 1 | Point, Area. Directional has none, w 0
	float4	Direction;		// Direction the light points at, w 0 | Directional, Area normal

	float3	Color;			// Linear colour
	uint	Type;			// One of LIGHT_TYPE_*

	float	Intensity;		// Brightness multiplier
	float	Range;			// Distance the light reaches | Point
	float	SourceRadius;	// How big the emitter is | Point, keeps the falloff finite at the centre
	int		ShadowSlot;		// Slice this light owns in the shadow atlas, NO_SHADOW_CASTER when it does not cast

	float2	Size;			// Width and height of the emitting surface | Area
	float	SpecularScale;	// Highlight strength of this light, 1 is neutral | all types
	float	SpotCosInner;	// Cosine of the inner cone half angle, full brightness inside it | Spot

	float3	TypeParams;		// Unused | reserved for whichever type needs it next
	float	SpotCosOuter;	// Cosine of the outer cone half angle, nothing past it | Spot
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
#define GBUFFER_VELOCITY			4
#define GBUFFER_TARGET_COUNT		5

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
	float2 Velocity			: SV_Target4;	// Where this pixel was last frame, in uv, see PrevWorldToClip
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
#define BASE_PASS_DEBUG_NORMAL_LENGTH		6
#define BASE_PASS_DEBUG_VELOCITY			7
#define BASE_PASS_DEBUG_COUNT				8


// ---------------------------------------------------------------------------------------
//  Tangent frame overlay
// ---------------------------------------------------------------------------------------

// Which vectors of the surface basis the overlay draws, one bit each. The bit of a vector is
// 1 shifted by its line number in the shader, so the order here is the order the vertex
// shader numbers them in: tangent first, bitangent second, normal third.
#define TANGENT_FRAME_SHOW_TANGENT			1
#define TANGENT_FRAME_SHOW_BITANGENT		2
#define TANGENT_FRAME_SHOW_NORMAL			4

// A mesh with this many vertices or fewer ignores the percentage and draws the frame of every
// one of them. A share of a handful of vertices is not worth sampling. The built in cube and plane
// ask for all of theirs regardless, see Primitive::AlwaysShowsAllFrameVertices. Only the CPU
// reads this, it hands the shader the verdict as ShowAllVertices.
#define TANGENT_FRAME_SHOW_ALL_VERTEX_LIMIT	20

// A hardware line is always one pixel wide, so a thicker one is a quad: two triangles, six
// corners, stretched across the line in screen space
#define TANGENT_FRAME_VERTICES_PER_LINE		6

// Lines drawn per vertex, and so the number of vertices a draw of one vertex takes: a quad
// for each of the three vectors
#define TANGENT_FRAME_LINES_PER_VERTEX		3
#define TANGENT_FRAME_VERTICES_PER_VERTEX	18


// ---------------------------------------------------------------------------------------
//  Origin axes overlay
// ---------------------------------------------------------------------------------------

// Three arrows for each object, along its X, Y and Z. Each is a shaft, a quad of six corners,
// and a head, a triangle of three, all built from SV_VertexID. There is no vertex buffer.
#define AXIS_GIZMO_SHAFT_CORNERS			6
#define AXIS_GIZMO_HEAD_CORNERS				3
#define AXIS_GIZMO_VERTICES_PER_ARROW		9
#define AXIS_GIZMO_ARROW_COUNT				3
#define AXIS_GIZMO_VERTEX_COUNT				27


// ---------------------------------------------------------------------------------------
//  Selection highlight
// ---------------------------------------------------------------------------------------

// The box drawn around the selected object: twelve edges, each a quad of six corners, built from SV_VertexID
#define SELECTION_BOX_EDGES					12
#define SELECTION_BOX_VERTICES				72


// ---------------------------------------------------------------------------------------
//  Transform gizmo
// ---------------------------------------------------------------------------------------

// What the handles do. TransformMode of Utility/TransformGizmo.h derives its values from these.
#define TRANSFORM_GIZMO_MODE_MOVE			0
#define TRANSFORM_GIZMO_MODE_ROTATE			1

// Axis index of "none", for the hovered and the held handle. 0, 1 and 2 are X, Y and Z.
#define TRANSFORM_GIZMO_AXIS_NONE			3

// A line is a quad of six corners, like every thick line of the overlays
#define TRANSFORM_GIZMO_VERTICES_PER_LINE	6

// A move handle is an arrow: a shaft of six corners and a head of three, the same as the origin axes
#define TRANSFORM_GIZMO_VERTICES_PER_ARROW	9
#define TRANSFORM_GIZMO_MOVE_VERTEX_COUNT	27

// A rotate handle is a ring, a closed loop of short thick lines. Sixty four of them are smooth
// to the eye at any size the ring is drawn at.
#define TRANSFORM_GIZMO_RING_SEGMENTS		64
#define TRANSFORM_GIZMO_VERTICES_PER_RING	384
#define TRANSFORM_GIZMO_RING_VERTEX_COUNT	1152

// The line the ring turns about, through the centre, one quad for each axis
#define TRANSFORM_GIZMO_AXIS_LINE_VERTICES	6
#define TRANSFORM_GIZMO_ROTATE_VERTEX_COUNT	1170


// ---------------------------------------------------------------------------------------
//  Constant buffers
//
//  C++ always gets every struct. HLSL only gets the ones its pass actually binds, which is
//  b1 carries a different buffer in each pass, so declaring all of them at once would put two
//  cbuffers on one register. Field names must be unique within a pass, an HLSL cbuffer puts
//  its fields in global scope.
//
//  ADDING OR EDITING A BUFFER | start every vector on a 16 byte boundary
//
//  HLSL lays fields into 16 byte rows and pushes a vector to the next row rather than let it
//  cross a boundary. C++ packs tightly and never does, so one field order can mean two
//  layouts and nothing diagnoses it. Scalars are exempt and may sit at any offset.
//
//      float2  A;      // HLSL 0    C++ 0
//      float3  B;      // HLSL 16   C++ 8     <-- eight bytes adrift
//
//  Pair a float3 with a scalar, or a float2 with two scalars, so each group fills a row
//  exactly; Light above does this four times and needs no padding. Where the order cannot
//  change, add an explicit padding member so C++ leaves the gap HLSL would. A matrix always
//  starts a row. alignas does not help, it sets the start address and the total size but
//  never the offsets in between. The struct size must still divide by 16, so that the next
//  element of an array lands on a row too.
// ---------------------------------------------------------------------------------------

// b0 | Per object. Rebound per draw, by the base pass, every shadow view and both overlays.
#if defined(__cplusplus) || defined(BASE_PASS) || defined(SHADOW_PASS) || defined(TANGENT_FRAME_PASS) || defined(AXIS_GIZMO_PASS) || defined(SELECTION_PASS)
CBUFFER_BEGIN(PrimitiveConstantBuffer, CB_PER_OBJECT)
	float4x4 LocalToWorld;				// Only with translation
	float4x4 WorldToClip;				// Translation * View * Proj (Clip)
	float4x4 NormalToWorld;				// Inverse transpose of LocalToWorld, so a normal stays perpendicular under a non uniform scale
	float4x4 PrevWorldToClip;			// Last frame's LocalToWorld * view projection | velocity, equals WorldToClip when still
CBUFFER_END
#endif

// b1 | Per view, light pass flavour. What the shading needs to know about the camera.
#if defined(__cplusplus) || defined(LIGHT_PASS)
CBUFFER_BEGIN(CameraConstantBuffer, CB_PER_VIEW)
	float4   ViewPosition;			// World space position of the camera
	float4   PreViewTranslation;	// -ViewPosition, add to a world position to translate it
	float4x4 ViewProj;				// World to clip
	float4x4 InvViewProj;			// Clip to world
	float4x4 ViewProjTranslated;	// Translated world to clip, as Unreal's TranslatedViewProjectionMatrix
	float4x4 InvViewProjTranslated;	// Clip to translated world, which is world plus PreViewTranslation
CBUFFER_END
#endif

// b1 | Per view, shadow pass flavour. Root constants rather than a CBV, so 16 rather than
// 256: there can be eight views in a frame and rebinding a buffer for each would be waste.
#if defined(__cplusplus) || defined(SHADOW_PASS)
CBUFFER_BEGIN(ShadowViewConstants, CB_PER_VIEW)
	float4x4 WorldToLightClip;		// World straight into this light's clip space
	float3   LightPosition;			// World space, a cube face measures distance from it
	float    LightRange;			// Distance is stored divided by this, so it fits 0 to 1
CBUFFER_END
#endif

// b1 | Per view, base pass flavour. A single value, so it rides in the root signature.
// Root constants, so no 16 byte rule: the size has to match the declared Num32BitValues.
#if defined(__cplusplus) || defined(BASE_PASS)
CBUFFER_BEGIN(BasePassDebugConstants, CB_PER_VIEW)
	uint DebugMode;					// One of BASE_PASS_DEBUG_*
CBUFFER_END
#endif

// b1 | Per view, tangent frame overlay flavour. Root constants like the shadow view, since
// the camera matrix changes every frame and the settings come straight out of the panel.
// The camera's own matrix is in here because the overlay has no camera buffer bound: it
// moves a vertex by a world space offset, which the per object matrices cannot do.
#if defined(__cplusplus) || defined(TANGENT_FRAME_PASS)
CBUFFER_BEGIN(TangentFrameConstants, CB_PER_VIEW)
	float4x4 WorldToCameraClip;		// World straight into the camera's clip space
	float    VectorLength;			// World units one unit vector is drawn at
	float    VertexFraction;		// Share of a mesh's vertices that draw their vectors, 0 to 1
	uint     VectorMask;			// Which vectors to draw, bits of TANGENT_FRAME_SHOW_*
	float    LineThickness;			// Pixels wide, a line is a quad this wide since hardware lines are always one
	float2   ViewportSize;			// Render target size in pixels, to turn a pixel width into clip space
	uint     ShowAllVertices;		// Non zero ignores VertexFraction and draws every vertex, set again for every draw
	float    PadTangentFrame0;		// Two scalars after a float2 fill its row, a whole number of rows like ShadowViewConstants
CBUFFER_END
#endif

// b1 | Per view, origin axes overlay flavour. Root constants, for the same reasons as the
// tangent frame's: the camera moves every frame and the rest comes straight out of the panel.
#if defined(__cplusplus) || defined(AXIS_GIZMO_PASS)
CBUFFER_BEGIN(AxisGizmoConstants, CB_PER_VIEW)
	float4x4 WorldToCameraClip;		// World straight into the camera's clip space
	float    AxisLength;			// World units one arrow runs from the origin
	float    LineThickness;			// Pixels wide the shaft is
	float    HeadSize;				// Pixels long the arrow head is, and half of that wide
	float    PadAxisGizmo0;			// To a whole row
	float2   ViewportSize;			// Render target size in pixels
	float    PadAxisGizmo1;			// Two scalars after a float2 fill its row, 96 bytes in all
	float    PadAxisGizmo2;
CBUFFER_END
#endif

// b1 | Per view, selection highlight flavour. Root constants. The box is given in the object's own
// space and goes through the per object matrices like a vertex of the mesh would, so it is
// turned and scaled with the object and the pass needs no camera matrix of its own.
#if defined(__cplusplus) || defined(SELECTION_PASS)
CBUFFER_BEGIN(SelectionConstants, CB_PER_VIEW)
	float3   BoxMin;				// Corner of the box with the smallest coordinates, object space
	float    SelectionLineThickness;// Pixels wide the box's edges are
	float3   BoxMax;				// Opposite corner
	float    PadSelection0;			// To a whole row
	float4   SelectionColor;		// rgb is the colour, a is how strongly it tints the surface. The box is drawn solid.
	float2   SelectionViewport;		// Render target size in pixels
	float    PadSelection1;			// 64 bytes in all
	float    PadSelection2;
CBUFFER_END
#endif

// b1 | Per view, transform gizmo flavour. Root constants, and the only per view data the pass has:
// the handles belong to no object, they sit in the world at the object's position.
#if defined(__cplusplus) || defined(TRANSFORM_GIZMO_PASS)
CBUFFER_BEGIN(TransformGizmoConstants, CB_PER_VIEW)
	float4x4 WorldToCameraClip;		// World straight into the camera's clip space
	float3   GizmoOrigin;			// World space, the centre of the handles
	float    GizmoLength;			// World units from the centre to the tip of an arrow, and the radius of a ring
	float3   GizmoAxisX;			// The three axes in the world, unit length. Either the world's own or the object's.
	uint     GizmoMode;				// One of TRANSFORM_GIZMO_MODE_*
	float3   GizmoAxisY;
	uint     GizmoHoveredAxis;		// 0 to 2, or TRANSFORM_GIZMO_AXIS_NONE
	float3   GizmoAxisZ;
	uint     GizmoActiveAxis;		// The held handle, same values
	float3   GizmoCameraPosition;	// Tells the near half of a ring from the far half
	float    GizmoLineThickness;	// Pixels wide
	float2   GizmoViewport;			// Render target size in pixels
	float    GizmoHeadSize;			// Pixels long an arrow head is
	float    PadTransformGizmo0;	// 160 bytes in all, a whole number of rows
CBUFFER_END
#endif

// b2 | Every light of the scene. Each one owns a slice of the shadow atlas; directional and
// area lights render a single depth view, so only those need a matrix here. A point light
// is looked up by direction, which is why its six face matrices never leave the shadow pass.
#if defined(__cplusplus) || defined(LIGHT_PASS)
CBUFFER_BEGIN(LightsConstantBuffer, CB_PER_LIGHTS)
	Light    Lights[MAX_LIGHTS];				// All lights of the scene
	float4x4 ShadowViewProj[MAX_LIGHTS];		// World to that light's clip space | Directional, Area
	uint     NumLights;							// How many entries of Lights are filled
	uint     PadLights0;						// To 592, the next multiple of 16 after 580
	uint     PadLights1;						// Three scalars, not uint[3]: HLSL gives every
	uint     PadLights2;						// array element a row of its own, so that would be 48
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
