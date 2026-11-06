//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_RENDERER_H_INCLUDED
#define D3D12_RENDERER_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIMath.h"
#include "Material.h"
#include "GBuffer.h"
#include "ShadowAtlas.h"
#include "CubeMap.h"
#include "AssetCatalog.h"

// Core | Everything every D3D12 app would otherwise duplicate
#include "DXIDevice.h"
#include "DXISwapChain.h"
#include "DXIDepthBuffer.h"
#include "DXIFrameContext.h"

// Objects
#include "Camera.h"
#include "Plane.h"
#include "Cube.h"
#include "Sphere.h"

// Lights
#include "DirectionalLight.h"
#include "PointLight.h"
#include "AreaLight.h"

// Passes
#include "ShadowPass.h"
#include "BasePass.h"
#include "LightPass.h"

// UI
#include "DebugUI.h"


// Which scene Initialize builds, chosen by -scene on the command line.
//
// Two very different things are worth having in front of the renderer. The built in scene
// is a handful of generated primitives with Poly Haven maps, which starts instantly and is
// where a lighting or shading change is easiest to read. Sponza is a quarter of a million
// triangles over twenty five materials that nothing in this engine authored, which is where
// a loader or a scaling assumption actually gets tested. Neither replaces the other.
enum class SceneSetup : UINT
{
	Default,	// Cubes, ground, sphere and Spot. What runs with no switch.
	Sponza,		// Sponza alone, at the origin, with the sun sized to cover it
	Both,		// Both at once, Sponza stood off along +Z so the two do not overlap
	CornellBox	// The Cornell Box, to its published measurements. Exclusive, it is a closed room.
};

// Deferred renderer.
// Same surface as D3D12App so Win32Proc can drive either of them, but the D3D12
// plumbing lives in the Core classes and the frame is a BasePass into the G-buffer
// followed by a LightPass that shades it.
class D3D12Renderer
{
	DISABLE_COPY_MOVE(D3D12Renderer)

public:

	D3D12Renderer() = delete;
	// Ctors & DCtors
	D3D12Renderer(UINT WindowWidth, UINT WindowHeight, std::wstring WindowName);
	~D3D12Renderer() = default; //just do nothing, the members take care of deallocation

	// Getters
	_NODISCARD UINT					GetWindowWidth() const	{ return WindowWidth; }
	_NODISCARD UINT					GetWindowHeight() const	{ return WindowHeight; }
	_NODISCARD float				GetAspectRatio() const	{ return AspectRatio; }
	_NODISCARD const std::wstring&	GetWindowName() const	{ return WindowName; }

	// Functions
	void Initialize();
	void Render();
	void Update(float DeltaTime);
	void Destroy();

	// Rebuilds everything that was sized to the window: the back buffers, the depth buffer,
	// the G-buffer, the views onto them, the viewport and the camera's projection.
	//
	// Called from WM_SIZE, which arrives for every mouse move of a drag, so Win32Proc holds
	// the final size back until the drag ends rather than rebuilding dozens of times.
	void OnResize(const UINT Width, const UINT Height);

	// Movement of camera
	void OnKeyDown(UINT8 Key);
	void OnKeyUp(UINT8 Key);
	void OnMouseMove(int DeltaX, int DeltaY);

	// Presentation | With vsync on the frame rate can never exceed the refresh rate of
	// the display, which is 32 Hz over a remote desktop session
	void SetVSyncEnabled(const bool bEnabled) noexcept
	{
		bVSyncEnabled = bEnabled;
	}
	_NODISCARD bool IsVSyncEnabled() const noexcept
	{
		return bVSyncEnabled;
	}

private:

	// Initialization, split per concern instead of one long function
	void InitializeConstantBuffers();
	void InitializeMaterials();
	void InitializeScene();
	void InitializePasses();

	// The setups InitializeScene picks between, plus the lights they share
	void InitializeDefaultScene();
	void InitializeSponzaScene();
	void InitializeCornellBoxScene();
	void InitializeSceneLights();

	// Which scene was asked for. Reads -scene, falling back to the built in one.
	_NODISCARD static SceneSetup GetRequestedScene();

	// Whether the active setup includes each half, so a conditional reads as a question
	// about the scene rather than as a comparison against an enum
	_NODISCARD bool UsesDefaultScene() const noexcept
	{
		return ActiveScene == SceneSetup::Default || ActiveScene == SceneSetup::Both;
	}
	_NODISCARD bool UsesSponzaScene() const noexcept
	{
		return ActiveScene == SceneSetup::Sponza || ActiveScene == SceneSetup::Both;
	}
	_NODISCARD bool UsesCornellBoxScene() const noexcept
	{
		return ActiveScene == SceneSetup::CornellBox;
	}

	// Loads one material out of a Poly Haven style folder, where every map is named
	// <AssetName>_<suffix>_1k.png, appends it to Materials and hands it back.
	// Missing maps keep their null descriptor. Ownership stays in Materials, the
	// reference is there so the caller can keep setting properties on what it just made.
	Material& LoadMaterial(const wchar_t* AssetName);

	// For a model that ships a single colour map rather than a folder of PBR maps. The
	// remaining slots take the neutral values SetDefaults fills in.
	Material& CreateMaterialWithDiffuse(const wchar_t* Name, const std::wstring& DiffusePath);

	// A flat untextured colour, for a scene whose surfaces are specified as reflectances
	// rather than shipped as images. Rgba is linear, not sRGB, matching SetSolidColor.
	Material& CreateMaterialWithColor(const wchar_t* Name, const BYTE (&Rgba)[4], const BYTE Roughness = 128);

	// Loads a catalog material the first time something asks for it and remembers where
	// it landed. Returns null when the shared heap is full or the index is out of range.
	Material* EnsureMaterialLoaded(const int CatalogIdx);

	// An already loaded material by name, or null. Lets the scene ask for one without
	// depending on the order InitializeMaterials happens to load them in.
	_NODISCARD Material* FindMaterial(const std::wstring& Name) const;

	// Appends an entire glTF file to the scene: one Material per material it declares and
	// one Model per primitive, placed by the transform given.
	//
	// Nothing like the other loaders, which produce one thing each. A glTF file is a scene
	// rather than an asset, so it arrives already split into pieces and already positioned
	// relative to itself, and the only transform worth applying is one that moves the whole
	// thing at once. Every piece therefore gets the same transform.
	//
	// Skipped with a log rather than fatal when the file is absent, like Spot: the assets
	// are downloaded rather than committed.
	void LoadGltfScene(const std::wstring& Path,
						const XMFLOAT3 Translation	= DX_IDENTITY_TRANSLATE3,
						const XMFLOAT3 Rotation		= DX_IDENTITY_ROTATE3,
						const XMFLOAT3 Scale		= DX_IDENTITY_SCALE3);

	// Drains whatever the panel asked for. Called once per frame, after BeginFrame, so
	// the command list is open for the vertex and texture uploads a spawn performs.
	void ProcessPendingSpawns();

	// Frame
	void BeginFrame();
	void EndFrame();

	// Passes
	void RenderShadowPass();
	void RenderBasePass();
	void RenderLightPass();
	void RenderDebugUI();

	// World to clip for one light, and whether it needs the cube pipeline.
	// A point light has six of these, one per cube face.
	_NODISCARD XMMATRIX GetLightShadowViewProj(const LightBase& Light, const UINT FaceIdx = 0) const;

	// Uploads
	void UploadCameraConstants();
	void UploadLightConstants();
	void UploadPrimitiveConstants(const Primitive& Primitive, const UINT PrimitiveIdx);

	// GPU address of a primitive's constants for the frame being recorded
	_NODISCARD D3D12_GPU_VIRTUAL_ADDRESS GetPrimitiveCbvAddress(const SIZE_T PrimitiveIdx) const;

	// Constants / Misc
	static constexpr UINT	BACK_BUFFER_COUNT	= 3;											//how many frames are in flight
	static constexpr UINT	MAX_MATERIALS		= 64;											//how many materials fit in the shared descriptor heap, Sponza alone declares 25
	static constexpr UINT	CAMERA_CBV_STRIDE	= CameraConstantBufferSize;							//already a multiple of the constant buffer alignment
	static constexpr UINT	SKY_FACE_RESOLUTION	= 512;												//per radiance cube face, projected out of the panorama on the CPU
	static constexpr UINT	SKY_IRRADIANCE_RESOLUTION = 32;											//irradiance barely varies with direction, so it stays tiny
	static constexpr UINT	SHADOW_MAP_RESOLUTION = 2048;										//per slice, for directional and area lights
	static constexpr UINT	SHADOW_CUBE_RESOLUTION = 512;										//per cube face, six of them add up fast

	// Window Properties
	UINT					WindowWidth;
	UINT					WindowHeight;
	float					AspectRatio;
	std::wstring			WindowName;

	// D3D12 Window Properties
	CD3DX12_VIEWPORT		ViewPort;
	CD3DX12_RECT			ScissorRect;

	// Core | Device, presentation, depth and command recording
	DXIDevice				Device;
	DXISwapChain			SwapChain;
	DXIDepthBuffer		DepthBuffer;
	DXIFrameContext		FrameContext;
	UINT					CurrentFrameIdx = 0;

	// Off by default, otherwise the frame rate is pinned to the refresh rate and says
	// nothing about how long the frame actually takes
	bool					bVSyncEnabled	= false;

	// Set by Destroy(), after which Update() and Render() do nothing
	bool					bDestroyed		= false;

	// Deferred rendering | ShadowPass writes depth from the light, BasePass writes the
	// surface, LightPass shades it and looks the occlusion up in the shadow map
	GBuffer					SceneGBuffer;
	ShadowAtlas				SceneShadowAtlas;

	// Sky removed for now. It was two CubeMaps here, a radiance cube serving as both the
	// background and the specular reflection source, and an irradiance cube for the
	// diffuse reflection. CubeMap.h still builds, nothing instantiates it.
	ShadowPass				SceneShadowPass;
	BasePass				SceneBasePass;
	LightPass				SceneLightPass;

	// Everything LightPass samples has to live in one shader visible heap, because only
	// one CBV/SRV/UAV heap can be bound at a time
	ComPtr<DXDescriptorHeap>	LightInputsHeap;

	// Materials | Every material takes its descriptors out of the one shared heap,
	// one per primitive so each object looks different
	MaterialHeap							SharedMaterialHeap;
	std::vector<std::unique_ptr<Material>>	Materials;

	// What a primitive is drawn with when nothing was assigned to it
	std::unique_ptr<Material>				DefaultMaterial;

	// Scene
	Camera									Camera;
	std::vector<std::unique_ptr<Primitive>>	Primitives;
	std::vector<std::unique_ptr<LightBase>>	Lights;

	// Settled once in Initialize, before anything is built, and read by the material and
	// light setup afterwards. Not changeable at runtime: swapping scenes means releasing
	// vertex buffers and textures the GPU may still be reading, which is a teardown path
	// that does not exist yet.
	SceneSetup								ActiveScene = SceneSetup::Default;

	// What is available to spawn, discovered on disk at startup
	AssetCatalog							Catalog;

	// Constant buffers | One region per frame in flight, all of them permanently mapped
	ComPtr<DXResource>		PrimitiveCBV;					// one PrimitiveConstantBuffer per primitive, per frame
	UINT8*					PrimitiveCbvAddress	= nullptr;
	ComPtr<DXResource>		CameraCBV;						// eye position, per frame
	UINT8*					CameraCbvAddress	= nullptr;
	ComPtr<DXResource>		LightsCBV;						// every light of the scene, per frame
	UINT8*					LightsCbvAddress	= nullptr;

	// Timer | Win32Proc hands Update() a zero delta, so the timer is what actually drives movement
	Helpers::StepTimer		StepTimer;

	// Debug panel drawn over the finished frame
	DebugUI					SceneDebugUI;
};

#endif // D3D12_RENDERER_H_INCLUDED
