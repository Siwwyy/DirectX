//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_DEBUGUI_H_INCLUDED
#define D3D12_DEBUGUI_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "../Objects/Camera.h"
#include "../Objects/Primitive.h"
#include "../Passes/BasePass.h"
#include "../Material.h"
#include "../LightTypes/LightBase.h"

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

	// Hooks ImGui up to the window and the device. The back buffer format has to match
	// what the UI is drawn into, and FrameCount the number of frames in flight.
	void Initialize(DXDevice * Device, const HWND WindowHandle, const DXGI_FORMAT BackBufferFormat, const UINT FrameCount);

	// Starts a UI frame, call it before Draw()
	void BeginFrame();

	// Fills the panel out of the live scene state. Primitives and lights come in by
	// non const reference because the panel edits them in place, the renderer picks the
	// changes up on its next upload.
	void Draw(const Camera& Camera,
				std::vector<std::unique_ptr<Primitive>>& Primitives,
				std::vector<std::unique_ptr<LightBase>>& Lights,
				const std::vector<std::unique_ptr<Material>>& Materials,
				Material * const DefaultMaterial,
				const UINT FramesPerSecond);

	// Records the UI draw calls. The caller has to have the back buffer bound already,
	// and this binds its own descriptor heap for the font.
	void Render(DXGraphicsCommandList * CommandList);

	// Forwards a window message to ImGui, returns true when the UI consumed it so the
	// camera does not also react to the same click or key
	_NODISCARD static bool HandleWindowMessage(const HWND WindowHandle, const UINT Message, const WPARAM WParam, const LPARAM LParam);

	// Whether the UI currently wants the mouse or the keyboard, so the camera can stay
	// still while a widget is being dragged
	_NODISCARD static bool WantsMouse();
	_NODISCARD static bool WantsKeyboard();

	// Getters
	_NODISCARD FORCEINLINE bool IsInitialized() const noexcept
	{
		return FontHeap != nullptr;
	}

	// Index of the primitive picked in the list, so the caller can act on the selection
	_NODISCARD FORCEINLINE int GetSelectedPrimitiveIdx() const noexcept
	{
		return SelectedPrimitiveIdx;
	}

	// Which surface debug view BasePass should write, picked in the panel
	_NODISCARD FORCEINLINE BasePassDebugMode GetBasePassDebugMode() const noexcept
	{
		return BasePassDebug;
	}

private:

	// A single descriptor, ImGui keeps its font atlas there
	ComPtr<DXDescriptorHeap>	FontHeap;

	// Draws the editable properties of one object
	void DrawPrimitiveProperties(Primitive& Primitive,
								const std::vector<std::unique_ptr<Material>>& Materials,
								Material * const DefaultMaterial);
	void DrawLightProperties(LightBase& Light);

	// Which entry of the object list is open
	int							SelectedPrimitiveIdx = 0;
	int							SelectedLightIdx = 0;

	// Surface debug view, off means normal rendering
	BasePassDebugMode			BasePassDebug = BasePassDebugMode::Off;
};

#endif // D3D12_DEBUGUI_H_INCLUDED
