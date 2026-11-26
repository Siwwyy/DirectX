//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_DEBUGUI_H_INCLUDED
#define D3D12_DEBUGUI_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIDevice.h"
#include "Camera.h"
#include "Primitive.h"
#include "Sphere.h"
#include "BasePass.h"
#include "TangentFramePass.h"
#include "AxisGizmoPass.h"
#include "SelectionPass.h"
#include "TransformGizmo.h"
#include "Material.h"
#include "LightBase.h"
#include "AssetCatalog.h"

#include <memory>
#include <vector>

// Dear ImGui panel drawn on top of the frame.
// Keeps every ImGui include out of the renderer, which only has to call BeginFrame(),
// Draw() and Render() around its own passes.
class DebugUI
{
	DISABLE_COPY_MOVE(DebugUI)

public:

	DebugUI()	= default;
	~DebugUI();

	/**
	 * Hooks ImGui up to the window and the device. The back buffer format has to match
	 * what the UI is drawn into, and FrameCount the number of frames in flight.
	 */
	void Init(const DXIDevice& Device, const HWND WindowHandle, const DXGI_FORMAT BackBufferFormat, const UINT FrameCount);

	/**
	 * Starts a UI frame, call it before Draw()
	 */
	void BeginFrame();

	// What the panel has asked to be added to the scene. The renderer drains these at the
	// top of its next frame rather than the panel spawning anything itself, because a
	// spawn records copy commands and the command list is only open mid frame.
	_NODISCARD std::vector<PrimitiveSpawnRequest>& GetPendingPrimitiveSpawns() noexcept
	{
		return PendingPrimitiveSpawns;
	}

	_NODISCARD std::vector<LightSpawnRequest>& GetPendingLightSpawns() noexcept
	{
		return PendingLightSpawns;
	}

	// Fills the panel out of the live scene state. The camera, primitives and lights come
	// in by non const reference because the panel edits them in place, the renderer picks
	// the changes up on its next upload.
	void Draw(Camera& Camera,
				const AssetCatalog& Catalog,
				std::vector<std::unique_ptr<Primitive>>& Primitives,
				std::vector<std::unique_ptr<LightBase>>& Lights,
				const std::vector<std::unique_ptr<Material>>& Materials,
				Material * const DefaultMaterial,
				const UINT FramesPerSecond);

	/**
	 * Records the UI draw calls. The caller has to have the back buffer bound already,
	 * and this binds its own descriptor heap for the font.
	 */
	void Render(DXGraphicsCommandList * CommandList);

	/**
	 * Forwards a window message to ImGui, returns true when the UI consumed it so the
	 * camera does not also react to the same click or key
	 */
	_NODISCARD static bool HandleWindowMessage(const HWND WindowHandle, const UINT Message, const WPARAM WParam, const LPARAM LParam);

	/**
	 * Whether the UI currently wants the mouse or the keyboard, so the camera can stay
	 * still while a widget is being dragged
	 */
	_NODISCARD static bool WantsMouse();
	_NODISCARD static bool WantsKeyboard();

	// Getters
	_NODISCARD FORCEINLINE bool IsInitialized() const noexcept
	{
		return FontHeap != nullptr;
	}

	// Index of the selected primitive, or -1 when nothing is. The list and a click in the view
	// both set it, so the caller can act on the selection whichever way it was made.
	_NODISCARD FORCEINLINE int GetSelectedPrimitiveIdx() const noexcept
	{
		return SelectedPrimitiveIdx;
	}
	void SetSelectedPrimitiveIdx(const int PrimitiveIdx) noexcept
	{
		SelectedPrimitiveIdx = PrimitiveIdx;
	}

	// How the selected object is marked, picked in the panel
	_NODISCARD FORCEINLINE const SelectionSettings& GetSelectionSettings() const noexcept
	{
		return Selection;
	}

	// Not const either, for a host that edits it with widgets of its own instead of this panel
	_NODISCARD FORCEINLINE SelectionSettings& GetSelectionSettings() noexcept
	{
		return Selection;
	}

	// The move and rotate handles. Not const: the renderer's hotkeys switch the mode too.
	_NODISCARD FORCEINLINE TransformGizmoSettings& GetTransformGizmoSettings() noexcept
	{
		return TransformGizmoConfig;
	}

	// Which surface debug view BasePass should write, picked in the panel
	_NODISCARD FORCEINLINE BasePassDebugMode GetBasePassDebugMode() const noexcept
	{
		return BasePassDebug;
	}

	// What the tangent frame overlay should draw, picked in the panel
	_NODISCARD FORCEINLINE const TangentFrameSettings& GetTangentFrameSettings() const noexcept
	{
		return TangentFrame;
	}

	// What the origin axes overlay should draw, picked in the panel
	_NODISCARD FORCEINLINE const AxisGizmoSettings& GetAxisGizmoSettings() const noexcept
	{
		return AxisGizmo;
	}

	// Not const, for a host that edits both with widgets of its own. The master switch of each
	// lives here and nowhere else, so without a way to reach it such a host can never turn them on.
	_NODISCARD FORCEINLINE TangentFrameSettings& GetTangentFrameSettings() noexcept
	{
		return TangentFrame;
	}

	_NODISCARD FORCEINLINE AxisGizmoSettings& GetAxisGizmoSettings() noexcept
	{
		return AxisGizmo;
	}

	/**
	 * Whether the shadow pass runs at all. Off skips the pass and reports every light as
	 * casting nothing, so the scene is lit flat.
	 */
	_NODISCARD FORCEINLINE bool AreShadowsEnabled() const noexcept
	{
		return bShadowsEnabled;
	}
	void SetShadowsEnabled(const bool bEnabled) noexcept
	{
		bShadowsEnabled = bEnabled;
	}

	/**
	 * Whether the Frame Dump button was pressed since the last call, and the request is cleared
	 * by asking, so one press dumps one frame.
	 */
	_NODISCARD bool ConsumeDumpRequest() noexcept
	{
		const bool bRequested = bDumpRequested;
		bDumpRequested = false;
		return bRequested;
	}

	/**
	 * What the panel shows under the button once a dump has finished, or failed.
	 */
	void SetDumpStatus(const std::wstring& Message, const bool bSuccess);

private:

	// A single descriptor, ImGui keeps its font atlas there
	ComPtr<DXDescriptorHeap>	FontHeap;

	// Draws the editable properties of one object
	void DrawPrimitiveProperties(Primitive& Primitive,
								const std::vector<std::unique_ptr<Material>>& Materials,
								Material * const DefaultMaterial);
	void DrawLightProperties(LightBase& Light);

	/**
	 * The pickers that fill PendingPrimitiveSpawns and PendingLightSpawns
	 */
	void DrawSpawnPanel(const AssetCatalog& Catalog, const SIZE_T PrimitiveCount, const SIZE_T LightCount);

	std::vector<PrimitiveSpawnRequest>	PendingPrimitiveSpawns;
	std::vector<LightSpawnRequest>		PendingLightSpawns;

	// Where the built in shapes sit in the mesh picker. Catalog meshes follow them, so a
	// list index of SPAWN_MESH_BUILTIN_COUNT or more is a catalog entry.
	static constexpr int SPAWN_MESH_CUBE			= 0;
	static constexpr int SPAWN_MESH_PLANE			= 1;
	static constexpr int SPAWN_MESH_SPHERE			= 2;
	static constexpr int SPAWN_MESH_BUILTIN_COUNT	= 3;

	// What the spawn pickers are currently set to
	int							SpawnMeshIdx		= 0;	// built in shapes first, then catalog models
	int							SpawnMaterialIdx	= 0;	// 0 default, 1+ catalog materials
	int							SpawnLightTypeIdx	= 0;
	UINT						SpawnSphereDensity	= Sphere::DEFAULT_DENSITY;

	// Which entry of the object list is open
	int							SelectedPrimitiveIdx = -1;	// -1 is nothing selected, which is how the scene starts
	int							SelectedLightIdx = 0;

	// How the selection is marked, and the handles that move or turn it
	SelectionSettings			Selection;
	TransformGizmoSettings		TransformGizmoConfig;

	// Surface debug view, off means normal rendering
	BasePassDebugMode			BasePassDebug = BasePassDebugMode::Off;

	// Tangent, bitangent and normal drawn as lines over the shaded frame
	TangentFrameSettings		TangentFrame;

	// Three arrows at the origin of each flagged object
	AxisGizmoSettings			AxisGizmo;

	// Frame dump to EXR, the renderer does the work and reports back through SetDumpStatus
	bool						bDumpRequested	= false;
	bool						bDumpSucceeded	= true;
	std::string					DumpStatus;				// UTF-8, what ImGui can print

	bool						bShadowsEnabled = false;	// Off for now, -shadows turns it back on at startup
};

#endif // D3D12_DEBUGUI_H_INCLUDED
