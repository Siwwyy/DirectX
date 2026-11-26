//Copyright, Damian Andrysiak 2026, All Rights Reserved.

// The engine as a Python module: `import dxi`.
//
// The renderer runs in the Python process, in the thread that calls it, and draws into a window
// the caller owns. That window is a native handle: a Qt widget's winId(), a Tk frame's id, or
// anything else with an HWND. The module never creates, shows or subclasses a window, so what
// happens to the window's messages, and when a frame is drawn, is the caller's.
//
//     r = dxi.Renderer(int(widget.winId()), widget.width(), widget.height())
//     r.tick()                      # one frame, from a timer or a paint event
//     r.resize(w, h)                # in pixels, as the window reports them
//     r.shutdown()                  # before the window goes away
//
// This file is deliberately outside DXI/, which CMake globs into the engine library: it is the
// only code that knows Python exists, and DXICore stays free of it.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "D3D12Renderer.h"
#include "CommandLine.h"

namespace py = pybind11;

namespace
{
	using Vec3 = std::array<float, 3>;

	/**
	 * Position, rotation and scale of an object as three numbers each. Rotation is Euler angles in
	 * degrees, X then Y then Z about the world axes, which is what the engine stores.
	 */
	struct PyTransform
	{
		Vec3 Position	= { 0.f, 0.f, 0.f };
		Vec3 Rotation	= { 0.f, 0.f, 0.f };
		Vec3 Scale		= { 1.f, 1.f, 1.f };
	};

	_NODISCARD XMFLOAT3 ToFloat3(const Vec3& Value) noexcept
	{
		return XMFLOAT3(Value[0], Value[1], Value[2]);
	}

	_NODISCARD Vec3 ToVec3(const XMFLOAT4& Value) noexcept
	{
		return { Value.x, Value.y, Value.z };
	}

	/**
	 * The renderer plus the checks that turn a bad call into a Python exception instead of a
	 * crash in the engine.
	 */
	class PyRenderer
	{
	public:

		PyRenderer(const uintptr_t WindowHandleValue,
					const UINT Width,
					const UINT Height,
					const std::wstring& Args,
					const bool bDebugUI)
		{
			const HWND WindowHandle = reinterpret_cast<HWND>(WindowHandleValue);

			if (!IsWindow(WindowHandle))
			{
				throw py::value_error("window_handle is not a window");
			}

			if (Width == 0 || Height == 0)
			{
				throw py::value_error("width and height have to be at least 1");
			}

			// The process command line is Python's, it says nothing about the engine, so the switches
			// come in as a string. The first parse of the process wins, a second renderer in the same
			// process runs with the switches of the first. Index 0 is the program name in a command line.
			DXICMD::Parse((L"dxi " + Args).c_str());

			Renderer = std::make_unique<D3D12Renderer>(Width, Height, L"dxi");
			Renderer->SetWindowHandle(WindowHandle);
			Renderer->SetDebugUIEnabled(bDebugUI);
			Renderer->SetStatsInWindowTitle(false);	// the window has a title of the caller's own

			// Takes a few seconds with a large scene, the shaders are compiled here. Throws
			// std::runtime_error on a failed D3D12 call, which Python sees as a RuntimeError.
			Renderer->Initialize();
		}

		// A renderer that is still running when the object is collected is torn down properly. The
		// caller should have called shutdown() already, while its window still existed.
		~PyRenderer()
		{
			Shutdown();
		}

		PyRenderer(const PyRenderer&)				= delete;
		PyRenderer& operator=(const PyRenderer&)	= delete;

		// Frame
		void Tick()
		{
			if (!IsAlive()) { return; }

			// Waits for the GPU on a full queue, nothing Python in there needs the interpreter
			py::gil_scoped_release Release;
			Renderer->Tick();
		}

		void Resize(const UINT Width, const UINT Height)
		{
			if (!IsAlive()) { return; }

			py::gil_scoped_release Release;
			Renderer->Resize(Width, Height);
		}

		// Waits for the GPU and stops all drawing. The renderer's memory stays until the object is
		// collected, settings read earlier stay valid, but nothing is drawn or changed any more.
		void Shutdown()
		{
			if (Renderer && Renderer->IsInitialized())
			{
				py::gil_scoped_release Release;
				Renderer->Destroy();
			}
		}

		_NODISCARD bool IsAlive() const noexcept
		{
			return Renderer && Renderer->IsInitialized();
		}

		// Input | Ignored once shut down, a timer can fire after the window has been closed
		void KeyDown(const UINT Key)						{ if (IsAlive()) { Renderer->OnKeyDown(static_cast<UINT8>(Key)); } }
		void KeyUp(const UINT Key)							{ if (IsAlive()) { Renderer->OnKeyUp(static_cast<UINT8>(Key)); } }
		void MouseLook(const int DeltaX, const int DeltaY)	{ if (IsAlive()) { Renderer->OnMouseMove(DeltaX, DeltaY); } }
		void LeftButtonDown(const int X, const int Y)		{ if (IsAlive()) { Renderer->OnLeftButtonDown(X, Y); } }
		void LeftButtonUp(const int X, const int Y)			{ if (IsAlive()) { Renderer->OnLeftButtonUp(X, Y); } }
		void CursorMove(const int X, const int Y)			{ if (IsAlive()) { Renderer->OnCursorMove(X, Y); } }

		// Scene
		_NODISCARD SIZE_T ObjectCount() const
		{
			return Get().GetPrimitiveCount();
		}

		_NODISCARD std::vector<std::wstring> ObjectNames() const
		{
			const D3D12Renderer& Engine = Get();

			std::vector<std::wstring> Names;
			Names.reserve(Engine.GetPrimitiveCount());

			for (SIZE_T Idx = 0; Idx < Engine.GetPrimitiveCount(); ++Idx)
			{
				Names.push_back(Engine.GetPrimitiveAt(Idx)->Name);
			}

			return Names;
		}

		_NODISCARD PyTransform GetTransform(const int Idx) const
		{
			const Primitive& Target = At(Idx);

			PyTransform Result;
			Result.Position	= ToVec3(Target.GetPosVector());
			Result.Rotation	= ToVec3(Target.GetRotVector());
			Result.Scale	= ToVec3(Target.GetScaleVector());
			return Result;
		}

		void SetTransform(const int Idx, const PyTransform& Value)
		{
			At(Idx).SetAbsoluteTransform(ToFloat3(Value.Position), ToFloat3(Value.Rotation), ToFloat3(Value.Scale));
		}

		_NODISCARD std::pair<bool, bool> GetOverlayFlags(const int Idx) const
		{
			const Primitive& Target = At(Idx);
			return { Target.bShowTangentFrame, Target.bShowOriginAxes };
		}

		void SetOverlayFlags(const int Idx, const std::optional<bool> TangentFrame, const std::optional<bool> OriginAxes)
		{
			Primitive& Target = At(Idx);

			if (TangentFrame)	{ Target.bShowTangentFrame	= *TangentFrame; }
			if (OriginAxes)		{ Target.bShowOriginAxes	= *OriginAxes; }
		}

		_NODISCARD int GetSelected() const
		{
			return Get().GetSelectedPrimitiveIdx();
		}

		void SetSelected(const int Idx)
		{
			Get().SelectPrimitive(Idx);
		}

		// Settings | Live, the engine reads them again every frame
		_NODISCARD TransformGizmoSettings& GetGizmoSettings()
		{
			return Get().GetTransformGizmoSettings();
		}

		_NODISCARD SelectionSettings& GetSelectionSettingsRef()
		{
			return Get().GetSelectionSettings();
		}

		_NODISCARD TangentFrameSettings& GetTangentFrameSettingsRef()
		{
			return Get().GetTangentFrameSettings();
		}

		_NODISCARD AxisGizmoSettings& GetAxisSettingsRef()
		{
			return Get().GetAxisGizmoSettings();
		}

		_NODISCARD UINT GetWidth() const	{ return Get().GetWindowWidth(); }
		_NODISCARD UINT GetHeight() const	{ return Get().GetWindowHeight(); }

		_NODISCARD bool GetVSync() const	{ return Get().IsVSyncEnabled(); }
		void SetVSync(const bool bEnabled)	{ Get().SetVSyncEnabled(bEnabled); }

	private:

		// The engine, or an exception for a renderer that never finished building
		_NODISCARD D3D12Renderer& Get() const
		{
			if (!Renderer)
			{
				throw std::runtime_error("the renderer was not created");
			}

			return *Renderer;
		}

		_NODISCARD Primitive& At(const int Idx) const
		{
			Primitive* const Target = Idx >= 0 ? Get().GetPrimitiveAt(static_cast<SIZE_T>(Idx)) : nullptr;

			if (Target == nullptr)
			{
				throw py::index_error("object index out of range");
			}

			return *Target;
		}

		std::unique_ptr<D3D12Renderer> Renderer;
	};
}

PYBIND11_MODULE(dxi, m)
{
	m.doc() = "DXI, a DirectX 12 deferred renderer, running in this process and drawing into a window you own.";
	m.attr("__version__") = "0.1";

	// What the gizmo's handles do and which axes they follow
	py::enum_<TransformMode>(m, "TransformMode")
		.value("Move",		TransformMode::Move)
		.value("Rotate",	TransformMode::Rotate);

	py::enum_<TransformSpace>(m, "TransformSpace")
		.value("World",		TransformSpace::World)
		.value("Local",		TransformSpace::Local);

	py::class_<PyTransform>(m, "Transform")
		.def(py::init([](const Vec3& Position, const Vec3& Rotation, const Vec3& Scale)
			{
				return PyTransform{ Position, Rotation, Scale };
			}),
			py::arg("position")	= Vec3{ 0.f, 0.f, 0.f },
			py::arg("rotation")	= Vec3{ 0.f, 0.f, 0.f },
			py::arg("scale")	= Vec3{ 1.f, 1.f, 1.f })
		.def_readwrite("position",	&PyTransform::Position)
		.def_readwrite("rotation",	&PyTransform::Rotation, "Euler angles in degrees")
		.def_readwrite("scale",		&PyTransform::Scale)
		.def("__repr__", [](const PyTransform& T)
			{
				char Buffer[192];
				sprintf_s(Buffer, "Transform(position=(%g, %g, %g), rotation=(%g, %g, %g), scale=(%g, %g, %g))",
					T.Position[0], T.Position[1], T.Position[2],
					T.Rotation[0], T.Rotation[1], T.Rotation[2],
					T.Scale[0], T.Scale[1], T.Scale[2]);
				return std::string(Buffer);
			});

	// Handed out by Renderer.gizmo and Renderer.selection as references into the engine, so an
	// assignment to one of these is the setting changed
	py::class_<TransformGizmoSettings>(m, "GizmoSettings")
		.def_readwrite("enabled",			&TransformGizmoSettings::bEnabled)
		.def_readwrite("mode",				&TransformGizmoSettings::Mode)
		.def_readwrite("space",				&TransformGizmoSettings::Space)
		.def_readwrite("size",				&TransformGizmoSettings::Size, "Share of the screen height")
		.def_readwrite("line_thickness",	&TransformGizmoSettings::LineThickness, "Pixels")
		.def_readwrite("head_size",			&TransformGizmoSettings::HeadSize, "Pixels")
		.def_readwrite("grab_radius",		&TransformGizmoSettings::GrabRadius, "Pixels")
		.def_readwrite("move_snap",			&TransformGizmoSettings::MoveSnap, "World units, while Ctrl is held")
		.def_readwrite("rotate_snap",		&TransformGizmoSettings::RotateSnap, "Degrees, while Ctrl is held");

	py::class_<SelectionSettings>(m, "SelectionSettings")
		.def_readwrite("enabled",			&SelectionSettings::bEnabled)
		.def_readwrite("show_tint",			&SelectionSettings::bShowTint)
		.def_readwrite("tint_strength",		&SelectionSettings::TintStrength)
		.def_readwrite("show_box",			&SelectionSettings::bShowBox)
		.def_readwrite("box_thickness",		&SelectionSettings::BoxThickness, "Pixels")
		.def_property("color",
			[](const SelectionSettings& S) { return Vec3{ S.Color.x, S.Color.y, S.Color.z }; },
			[](SelectionSettings& S, const Vec3& Value) { S.Color = XMFLOAT3(Value[0], Value[1], Value[2]); });

	// The master switch of each overlay is `enabled`. Off, nothing is drawn whichever objects are
	// flagged, see Renderer.set_overlay_flags
	py::class_<TangentFrameSettings>(m, "TangentFrameSettings")
		.def_readwrite("enabled",			&TangentFrameSettings::bEnabled)
		.def_readwrite("show_tangent",		&TangentFrameSettings::bShowTangent, "Red, along growing U")
		.def_readwrite("show_bitangent",	&TangentFrameSettings::bShowBitangent, "Green, along growing V")
		.def_readwrite("show_normal",		&TangentFrameSettings::bShowNormal, "Blue, out of the surface")
		.def_readwrite("depth_test",		&TangentFrameSettings::bDepthTest)
		.def_readwrite("vector_length",		&TangentFrameSettings::VectorLength, "World units")
		.def_readwrite("line_thickness",	&TangentFrameSettings::LineThickness, "Pixels")
		.def_readwrite("vertex_percent",	&TangentFrameSettings::VertexPercent,
			"Share of a dense mesh's vertices that draw, small meshes always draw all");

	py::class_<AxisGizmoSettings>(m, "AxesSettings")
		.def_readwrite("enabled",			&AxisGizmoSettings::bEnabled)
		.def_readwrite("depth_test",		&AxisGizmoSettings::bDepthTest)
		.def_readwrite("axis_length",		&AxisGizmoSettings::AxisLength, "World units")
		.def_readwrite("line_thickness",	&AxisGizmoSettings::LineThickness, "Pixels")
		.def_readwrite("head_size",			&AxisGizmoSettings::HeadSize, "Pixels");

	py::class_<PyRenderer>(m, "Renderer",
		"The renderer. Construction builds the device, the swap chain and the scene, which compiles "
		"the shaders and takes a few seconds. Call every method from the thread that created it.")
		.def(py::init<uintptr_t, UINT, UINT, std::wstring, bool>(),
			py::arg("window_handle"), py::arg("width"), py::arg("height"),
			py::arg("args") = std::wstring(),
			py::arg("debug_ui") = false,
			"window_handle: an HWND as an int, for instance int(widget.winId()). width and height are the "
			"window's client size in pixels. args: engine switches written as on a command line, for "
			"instance '-nobreak -scene sponza', read once per process. debug_ui: the ImGui overlay, which "
			"only reacts to input that is forwarded to it.")

		// Frame
		.def("tick",		&PyRenderer::Tick,		"Updates and draws one frame")
		.def("resize",		&PyRenderer::Resize,	py::arg("width"), py::arg("height"),
			"Follows the window to a new client size in pixels. Zero pauses drawing, as when minimized.")
		.def("shutdown",	&PyRenderer::Shutdown,	"Waits for the GPU and stops. Call it before the window is destroyed.")
		.def_property_readonly("alive", &PyRenderer::IsAlive)
		.def_property_readonly("width",	&PyRenderer::GetWidth)
		.def_property_readonly("height", &PyRenderer::GetHeight)
		.def_property("vsync", &PyRenderer::GetVSync, &PyRenderer::SetVSync)

		// Input | Positions are client pixels, keys are Windows virtual key codes
		.def("key_down",			&PyRenderer::KeyDown,		py::arg("vk"))
		.def("key_up",				&PyRenderer::KeyUp,			py::arg("vk"))
		.def("mouse_look",			&PyRenderer::MouseLook,		py::arg("dx"), py::arg("dy"),
			"Turns the camera by a mouse movement in pixels, call it while the right button is held")
		.def("left_button_down",	&PyRenderer::LeftButtonDown,py::arg("x"), py::arg("y"),
			"Grabs a gizmo handle under the cursor, or else picks the object there")
		.def("left_button_up",		&PyRenderer::LeftButtonUp,	py::arg("x"), py::arg("y"))
		.def("cursor_move",			&PyRenderer::CursorMove,	py::arg("x"), py::arg("y"),
			"Drags a held handle, or lights the one under the cursor")

		// Scene
		.def_property_readonly("object_count", &PyRenderer::ObjectCount)
		.def("object_names",		&PyRenderer::ObjectNames)
		.def("get_transform",		&PyRenderer::GetTransform,	py::arg("index"))
		.def("set_transform",		&PyRenderer::SetTransform,	py::arg("index"), py::arg("transform"))
		.def("get_overlay_flags",	&PyRenderer::GetOverlayFlags, py::arg("index"),
			"(tangent_frame, origin_axes) of an object")
		.def("set_overlay_flags",	&PyRenderer::SetOverlayFlags, py::arg("index"),
			py::arg("tangent_frame") = std::nullopt, py::arg("origin_axes") = std::nullopt)
		.def_property("selected",	&PyRenderer::GetSelected, &PyRenderer::SetSelected,
			"Index of the selected object, -1 for none. A click in the view changes it too.")

		// Settings
		.def_property_readonly("gizmo",		&PyRenderer::GetGizmoSettings,		py::return_value_policy::reference_internal)
		.def_property_readonly("selection",	&PyRenderer::GetSelectionSettingsRef,	py::return_value_policy::reference_internal)
		.def_property_readonly("tangent_frame",	&PyRenderer::GetTangentFrameSettingsRef,	py::return_value_policy::reference_internal)
		.def_property_readonly("axes",		&PyRenderer::GetAxisSettingsRef,		py::return_value_policy::reference_internal)

		// Debug overlay | Only for a renderer made with debug_ui=True, whose window messages the host
		// forwards. Without a UI they report false.
		.def_static("handle_window_message",
			[](const uintptr_t Hwnd, const UINT Message, const uintptr_t WParam, const intptr_t LParam)
			{
				return DebugUI::HandleWindowMessage(reinterpret_cast<HWND>(Hwnd), Message, static_cast<WPARAM>(WParam), static_cast<LPARAM>(LParam));
			},
			py::arg("hwnd"), py::arg("message"), py::arg("wparam"), py::arg("lparam"))
		.def_static("wants_mouse",		&DebugUI::WantsMouse)
		.def_static("wants_keyboard",	&DebugUI::WantsKeyboard);
}
