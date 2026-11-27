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
#include "SpotLight.h"

// Passes
#include "ShadowPass.h"
#include "BasePass.h"
#include "LightPass.h"
#include "TangentFramePass.h"
#include "AxisGizmoPass.h"
#include "SelectionPass.h"
#include "TransformGizmoPass.h"
#include "FrameDump.h"

// Selection | Picking with the mouse, and the handles that move or turn what was picked
#include "Picking.h"
#include "TransformGizmo.h"

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

	/**
	 * Hosting | The renderer draws into a window it does not own: Win32Proc's for the executable,
	 * a Qt widget's for the editor. Whoever creates the window hands its handle over, once, before
	 * Initialize. The renderer never creates, shows or subclasses a window, and a host that wants
	 * the debug panel's input has to forward messages to DebugUI itself.
	 */
	void SetWindowHandle(const HWND InWindowHandle) noexcept
	{
		WindowHandle = InWindowHandle;
	}

	/**
	 * Hosting | The ImGui panel is an overlay, and a host with panels of its own does not want it.
	 * It has to be decided before Initialize, which only sets ImGui up when it is on. Off, the
	 * selection and gizmo settings still exist with their defaults, they just have no widgets.
	 */
	void SetDebugUIEnabled(const bool bEnabled) noexcept
	{
		bDebugUIEnabled = bEnabled;
	}

	/**
	 * Hosting | Frame rate and vsync state are written into the window's title by default, which
	 * is right for the executable. A host with its own title bar turns it off.
	 */
	void SetStatsInWindowTitle(const bool bEnabled) noexcept
	{
		bStatsInWindowTitle = bEnabled;
	}

	// Functions
	void Initialize();
	void Render();
	void Update(float DeltaTime);
	void Destroy();

	/**
	 * One whole frame: Update, then Render. What a host calls from its own timer or paint event.
	 */
	void Tick()
	{
		Update(0.f);
		Render();
	}

	/**
	 * Follows the window to a new client size in pixels: the swap chain, depth, G-buffer, viewport
	 * and the camera's aspect ratio. Waits for the GPU, so it is a hitch, not something to call
	 * every frame. A size of zero, which a minimized window reports, pauses rendering until a
	 * real size comes back. Before Initialize it does nothing, the size given to the constructor
	 * is what gets built.
	 */
	void Resize(const UINT NewWidth, const UINT NewHeight);

	_NODISCARD bool IsInitialized() const noexcept
	{
		return bInitialized && !bDestroyed;
	}

	// Movement of camera
	void OnKeyDown(UINT8 Key);
	void OnKeyUp(UINT8 Key);
	void OnMouseMove(int DeltaX, int DeltaY);

	// Selection | Positions are client pixels, as the window reports them. A press grabs a handle
	// of the selected object if the cursor is on one, and picks the object under it otherwise.
	void OnLeftButtonDown(int X, int Y);
	void OnLeftButtonUp(int X, int Y);
	void OnCursorMove(int X, int Y);

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

	/**
	 * Scene access | What a host with panels of its own needs to list and edit the scene. An index is
	 * a primitive's place in the scene, the one the selection and the debug panel use too, and it
	 * only stays valid until something is spawned or removed.
	 */
	_NODISCARD SIZE_T GetPrimitiveCount() const noexcept
	{
		return Primitives.size();
	}

	// Null when the index is out of range
	_NODISCARD Primitive* GetPrimitiveAt(const SIZE_T PrimitiveIdx) const noexcept
	{
		return PrimitiveIdx < Primitives.size() ? Primitives[PrimitiveIdx].get() : nullptr;
	}

	// -1 when nothing is selected
	_NODISCARD int GetSelectedPrimitiveIdx() const noexcept
	{
		return SceneDebugUI.GetSelectedPrimitiveIdx();
	}

	/**
	 * Selects a primitive, or nothing for an index that is -1 or out of range. A held gizmo handle is
	 * let go of, it belonged to the object that was selected before.
	 */
	void SelectPrimitive(const int PrimitiveIdx) noexcept
	{
		const bool bValid = PrimitiveIdx >= 0 && static_cast<SIZE_T>(PrimitiveIdx) < Primitives.size();

		SceneTransformGizmo.EndDrag();
		SceneTransformGizmo.ClearHover();
		SceneDebugUI.SetSelectedPrimitiveIdx(bValid ? PrimitiveIdx : -1);
	}

	/**
	 * Removes primitives from the scene, at once: it waits for the GPU, since the frames still in flight
	 * read the buffers of what goes. Call it between frames, never from inside one. Every primitive behind
	 * a removed one moves up, so the indices of the host's own lists are stale afterwards, shift them by the
	 * number of removed indices below each. The selection is shifted here, or cleared when it was removed.
	 * Indices that are out of range or repeated are ignored.
	 *
	 * @return How many primitives were removed
	 */
	SIZE_T DeletePrimitives(std::vector<int> PrimitiveIndices);

	/**
	 * The settings the overlays read, live: a change takes effect on the next frame. They exist and
	 * keep their defaults when the ImGui panel is off, a host's own widgets edit them here.
	 */
	_NODISCARD TransformGizmoSettings& GetTransformGizmoSettings() noexcept
	{
		return SceneDebugUI.GetTransformGizmoSettings();
	}

	_NODISCARD SelectionSettings& GetSelectionSettings() noexcept
	{
		return SceneDebugUI.GetSelectionSettings();
	}

	// Both overlays are off until their master switch is, whatever the objects' own flags say
	_NODISCARD TangentFrameSettings& GetTangentFrameSettings() noexcept
	{
		return SceneDebugUI.GetTangentFrameSettings();
	}

	_NODISCARD AxisGizmoSettings& GetAxisGizmoSettings() noexcept
	{
		return SceneDebugUI.GetAxisGizmoSettings();
	}

	/**
	 * What can be added to the scene | The meshes and materials found on disk, and the requests that
	 * add one. The ImGui panel files the same requests. A request is carried out at the start of the
	 * next frame, the new object is in front of the camera and last in the scene, and a request past
	 * MAX_PRIMITIVES or MAX_LIGHTS is dropped.
	 */
	_NODISCARD const AssetCatalog& GetCatalog() const noexcept
	{
		return Catalog;
	}

	_NODISCARD SIZE_T GetLightCount() const noexcept
	{
		return Lights.size();
	}

	/**
	 * @return The light at a position of the scene's light list, or null past the end
	 */
	_NODISCARD const LightBase* GetLight(const SIZE_T Idx) const noexcept
	{
		return Idx < Lights.size() ? Lights[Idx].get() : nullptr;
	}

	void RequestSpawnPrimitive(const PrimitiveSpawnRequest& Request)
	{
		SceneDebugUI.GetPendingPrimitiveSpawns().push_back(Request);
	}

	void RequestSpawnLight(const LightSpawnRequest& Request)
	{
		SceneDebugUI.GetPendingLightSpawns().push_back(Request);
	}

	/**
	 * Has an object wear a catalog material, from the next frame on. The material is loaded then if it
	 * was not yet. An unknown object or catalog index changes nothing.
	 */
	void RequestAssignMaterial(const MaterialAssignRequest& Request)
	{
		PendingMaterialAssignments.push_back(Request);
	}

private:

	/**
	 * Initialization, split per concern instead of one long function
	 */
	void InitializeConstantBuffers();
	void InitializeMaterials();
	void InitializeScene();
	void InitializePasses();

	/**
	 * Writes the views LightPass samples into LightInputsHeap: the G-buffer targets, the depth
	 * and the shadow atlas. Anything that recreates one of those resources has to call this again,
	 * a view outlives the resource it points at otherwise.
	 */
	void WriteLightInputViews();

	/**
	 * The ImGui panel is drawn only when the host wants it and it was set up
	 */
	_NODISCARD bool UsesDebugUI() const noexcept
	{
		return bDebugUIEnabled && SceneDebugUI.IsInitialized();
	}

	/**
	 * The setups InitializeScene picks between, plus the lights they share
	 */
	void InitializeDefaultScene();
	void InitializeSponzaScene();
	void InitializeCornellBoxScene();
	void InitializeSceneLights();

	/**
	 * Which scene was asked for. Reads -scene, falling back to the built in one.
	 */
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

	/**
	 * Loads one material out of a Poly Haven style folder, where every map is named
	 * <AssetName>_<suffix>_1k.png, appends it to Materials and hands it back.
	 * Missing maps keep their null descriptor. Ownership stays in Materials, the
	 * reference is there so the caller can keep setting properties on what it just made.
	 */
	Material& LoadMaterial(const wchar_t* AssetName);

	/**
	 * For a model that ships a single colour map rather than a folder of PBR maps. The
	 * remaining slots take the neutral values SetDefaults fills in.
	 */
	Material& CreateMaterialWithDiffuse(const wchar_t* Name, const std::wstring& DiffusePath);

	/**
	 * A flat untextured colour, for a scene whose surfaces are specified as reflectances
	 * rather than shipped as images. Rgba is linear, not sRGB, matching SetSolidColor.
	 */
	Material& CreateMaterialWithColor(const wchar_t* Name, const BYTE (&Rgba)[4], const BYTE Roughness = 128);

	/**
	 * Loads a catalog material the first time something asks for it and remembers where
	 * it landed. Returns null when the shared heap is full or the index is out of range.
	 */
	Material* EnsureMaterialLoaded(const int CatalogIdx);

	/**
	 * An already loaded material by name, or null. Lets the scene ask for one without
	 * depending on the order InitializeMaterials happens to load them in.
	 */
	_NODISCARD Material* FindMaterial(const std::wstring& Name) const;

	// Appends a whole glTF file: one Material per material, one Model per primitive, every
	// piece taking the same transform since the file already positions them relative to
	// each other. Logged and skipped when absent, the assets are downloaded not committed.
	void LoadGltfScene(const std::wstring& Path,
						const XMFLOAT3 Translation	= DX_IDENTITY_TRANSLATE3,
						const XMFLOAT3 Rotation		= DX_IDENTITY_ROTATE3,
						const XMFLOAT3 Scale		= DX_IDENTITY_SCALE3);

	/**
	 * Drains whatever the panel asked for. Called once per frame, after BeginFrame, so
	 * the command list is open for the vertex and texture uploads a spawn performs.
	 */
	void ProcessPendingSpawns();

	/**
	 * Where a drop at a pixel of the view lands: the first surface of the scene along the ray under it,
	 * or else the ground at height 0. False when the ray meets neither, or only something far off, as
	 * when the cursor is over the sky, and the spawn then goes in front of the camera as usual.
	 */
	_NODISCARD bool FindDropPoint(const int CursorX, const int CursorY, XMFLOAT3& OutPoint);

	/**
	 * Takes ownership of a light, keeping the sun at index 0.
	 *
	 * The position in Lights is the shadow atlas slice and the Lights[] slot the shaders
	 * read, so the order here is the order everything downstream sees. A directional light
	 * goes to the front, so a shader reaches the sun as Lights[SUN_LIGHT_INDEX] rather than
	 * searching for it. Only the first one does, a second appends like any other light.
	 *
	 * @param Light Light to place in the scene
	 */
	void AddLight(std::unique_ptr<LightBase> Light);

	/**
	 * @return Whether index 0 holds a directional light, which is what makes it the sun
	 */
	_NODISCARD bool HasSunLight() const noexcept;

	// Frame
	void BeginFrame();
	void EndFrame();

	// Passes
	void RenderShadowPass();
	void RenderBasePass();
	void RenderLightPass();
	void RenderTangentFramePass();
	void RenderAxisGizmoPass();
	void RenderSelectionPass();
	void RenderTransformGizmoPass();
	void RenderDebugUI();

	/**
	 * The selected primitive, or null when nothing is. Reads the panel's index so the list and
	 * a click in the view always agree, and checks it against the scene.
	 */
	_NODISCARD Primitive* GetSelectedPrimitive() const;

	/**
	 * The camera as the gizmo's maths wants it, and a client position turned into a viewport one
	 */
	_NODISCARD GizmoCamera BuildGizmoCamera() const;
	_NODISCARD XMFLOAT2 ClientToViewport(const int X, const int Y) const;

	/**
	 * EXR dump of every buffer. Capture records the copies into the frame being drawn when the
	 * panel asked for one, Finish runs after the frame was submitted, waits for the GPU and writes the files.
	 */
	void CaptureFrameDump();
	void FinishFrameDump();

	/**
	 * World to clip for one light, and whether it needs the cube pipeline.
	 * A point light has six of these, one per cube face.
	 */
	_NODISCARD XMMATRIX GetLightShadowViewProj(const LightBase& Light, const UINT FaceIdx = 0) const;

	// Uploads
	void UploadCameraConstants();
	void UploadLightConstants();
	void UploadPrimitiveConstants();

	/**
	 * GPU address of a primitive's constants for the frame being recorded
	 */
	_NODISCARD D3D12_GPU_VIRTUAL_ADDRESS GetPrimitiveCbvAddress(const SIZE_T PrimitiveIdx) const;

	/**
	 * Rolls every transform the velocity target depends on forward by one frame, the
	 * primitives and the camera alike.
	 *
	 * Runs after the constants have been uploaded, so this frame still wrote the previous
	 * values. Doing it earlier makes every pixel report zero motion, which renders fine
	 * and is therefore easy to miss.
	 */
	void VelocityPass();

	// Constants / Misc
	static constexpr UINT	BACK_BUFFER_COUNT	= 3;											//how many frames are in flight
	static constexpr UINT	MAX_MATERIALS		= 64;											//how many materials fit in the shared descriptor heap, Sponza alone declares 25
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

	bool					bVSyncEnabled	= false;	// Off by default, a pinned frame rate says nothing about frame cost
	bool					bDestroyed		= false;	// Set by Destroy(), after which Update() and Render() do nothing

	// Hosting | The window is somebody else's, see SetWindowHandle
	HWND					WindowHandle	= nullptr;
	bool					bInitialized	= false;	// Set at the end of Initialize, a resize or a frame before it has nothing to work on
	bool					bMinimized		= false;	// The window has no area, so nothing is rendered until it has again
	bool					bDebugUIEnabled	= true;		// Whether the ImGui overlay exists at all
	bool					bStatsInWindowTitle = true;	// Whether Update writes the frame rate into the title

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
	TangentFramePass		SceneTangentFramePass;	// Overlay on top of the shaded frame, only when the panel asks for it
	AxisGizmoPass			SceneAxisGizmoPass;		// Same, three arrows at the origin of each flagged object
	SelectionPass			SceneSelectionPass;		// Same, the selected object tinted and boxed
	TransformGizmoPass		SceneTransformGizmoPass;// Same, the arrows or rings that move or turn it

	// The mouse's side of selection. The state is the gizmo's, the passes above only draw it.
	TransformGizmo			SceneTransformGizmo;
	XMFLOAT2				CursorPixels	= { -1.f, -1.f };	// Last cursor position in the viewport, off screen until the mouse moves

	FrameDump				SceneFrameDump;			// Writes every buffer of a frame to EXR when the panel asks
	bool					bFrameDumpRecorded = false;	// Whether this frame's list holds the copies, so EndFrame knows to wait

	ComPtr<DXDescriptorHeap>	LightInputsHeap;	// One heap for everything LightPass samples, only one can be bound at a time

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

	// Material changes asked for since the last frame, see RequestAssignMaterial
	std::vector<MaterialAssignRequest>		PendingMaterialAssignments;

	// Number in the name of the next spawned primitive, see ProcessPendingSpawns
	SIZE_T									SpawnOrdinal = 0;

	// Constant buffers | One region per frame in flight, all of them permanently mapped
	ComPtr<DXResource>		PrimitiveCBV;					// one PrimitiveConstantBuffer per primitive, per frame
	UINT8*					PrimitiveCbvAddress	= nullptr;
	ComPtr<DXResource>		CameraCBV;						// view position, per frame
	UINT8*					CameraCbvAddress	= nullptr;
	ComPtr<DXResource>		LightsCBV;						// every light of the scene, per frame
	UINT8*					LightsCbvAddress	= nullptr;

	// Timer | Win32Proc hands Update() a zero delta, so the timer is what actually drives movement
	Helpers::StepTimer		StepTimer;

	// Debug panel drawn over the finished frame
	DebugUI					SceneDebugUI;
};

#endif // D3D12_RENDERER_H_INCLUDED
