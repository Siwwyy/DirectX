//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "DebugUI.h"

#include "DirectionalLight.h"
#include "PointLight.h"
#include "AreaLight.h"

#include <string>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// Namespaces
using namespace Helpers;

// Declared by the Win32 backend for exactly this, forwarding messages into ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	// The engine names everything in wide strings, ImGui speaks utf8
	void ToUtf8(const std::wstring& Wide, char* Out, const int OutSize)
	{
		WideCharToMultiByte(CP_UTF8, 0, Wide.c_str(), -1, Out, OutSize - 1, nullptr, nullptr);
	}

	// Prints one row of a matrix, the panel shows four of these per matrix
	void DrawMatrixRow(const char* Label, const float Row[4])
	{
		ImGui::Text("%s  %8.3f  %8.3f  %8.3f  %8.3f", Label, Row[0], Row[1], Row[2], Row[3]);
	}

	void DrawMatrix(const XMFLOAT4X4& Matrix)
	{
		DrawMatrixRow("r0", Matrix.m[0]);
		DrawMatrixRow("r1", Matrix.m[1]);
		DrawMatrixRow("r2", Matrix.m[2]);
		DrawMatrixRow("r3", Matrix.m[3]);
	}
}

DebugUI::~DebugUI()
{
	if (!IsInitialized())
	{
		return;
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void DebugUI::Init(DXDevice * Device, const HWND WindowHandle, const DXGI_FORMAT BackBufferFormat, const UINT FrameCount)
{
	DXASSERT(Device, "Device can not be null for Initialization of the debug UI");
	DXASSERT(WindowHandle, "Debug UI needs the window it draws into");
	DXASSERT(!IsInitialized(), "Debug UI has been already initialized");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// ImGui keeps its font atlas in a shader visible descriptor, one is enough
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc	= {};
	HeapDesc.NumDescriptors				= 1;
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&FontHeap)));
	NAME_D3D12_OBJECT(FontHeap, L"Debug UI Font Heap");

	ImGui_ImplWin32_Init(WindowHandle);
	ImGui_ImplDX12_Init(Device,
						static_cast<int>(FrameCount),
						BackBufferFormat,
						FontHeap.Get(),
						FontHeap->GetCPUDescriptorHandleForHeapStart(),
						FontHeap->GetGPUDescriptorHandleForHeapStart());
}

void DebugUI::BeginFrame()
{
	if (!IsInitialized())
	{
		return;
	}

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void DebugUI::DrawPrimitiveProperties(Primitive& Primitive,
									const std::vector<std::unique_ptr<Material>>& Materials,
									Material * const DefaultMaterial)
{
	// ImGui identifies a widget by hashing its label, and the light panel below has a
	// "Position" of its own in the same window. Without a scope here the two hash to one
	// id and dragging either moves whichever was submitted first. Keying the scope on the
	// object also keeps a half finished edit from following the selection to the next one.
	ImGui::PushID(&Primitive);

	// Transform. The matrices are rebuilt from these three, so editing any of them shows
	// up on the next upload the renderer does.
	auto Position	= Primitive.GetPosVector();
	auto Rotation	= Primitive.GetRotVector();
	auto Scale		= Primitive.GetScaleVector();

	float PositionXyz[3]	= { Position.x, Position.y, Position.z };
	float RotationXyz[3]	= { Rotation.x, Rotation.y, Rotation.z };
	float ScaleXyz[3]		= { Scale.x, Scale.y, Scale.z };

	bool bTransformChanged = false;
	bTransformChanged |= ImGui::DragFloat3("Position", PositionXyz, 0.05f);
	bTransformChanged |= ImGui::DragFloat3("Rotation", RotationXyz, 0.5f);
	bTransformChanged |= ImGui::DragFloat3("Scale", ScaleXyz, 0.05f, 0.001f, 1000.f);

	if (bTransformChanged)
	{
		// Absolute, not a nudge, because the widgets hand over where it should end up
		Primitive.SetTransform({ PositionXyz[0], PositionXyz[1], PositionXyz[2] },
								{ RotationXyz[0], RotationXyz[1], RotationXyz[2] },
								{ ScaleXyz[0],    ScaleXyz[1],    ScaleXyz[2] });
	}

	// Material, picked out of the ones already loaded. Entry 0 is the default every
	// primitive falls back to when nothing is assigned.
	ImGui::SeparatorText("Material");

	std::vector<std::string> MaterialNames;
	std::vector<Material*>   MaterialPtrs;

	MaterialNames.emplace_back("Default");
	MaterialPtrs.emplace_back(DefaultMaterial);

	for (const auto& Candidate : Materials)
	{
		char Name[128] = {};
		ToUtf8(Candidate->GetName(), Name, sizeof(Name));
		MaterialNames.emplace_back(Name);
		MaterialPtrs.emplace_back(Candidate.get());
	}

	// Which of them this primitive is currently using
	int CurrentMaterial = 0;
	for (int MaterialIdx = 0; MaterialIdx < static_cast<int>(MaterialPtrs.size()); ++MaterialIdx)
	{
		if (MaterialPtrs[MaterialIdx] == Primitive.GetMaterial())
		{
			CurrentMaterial = MaterialIdx;
			break;
		}
	}

	if (ImGui::BeginCombo("##Material", MaterialNames[CurrentMaterial].c_str()))
	{
		for (int MaterialIdx = 0; MaterialIdx < static_cast<int>(MaterialPtrs.size()); ++MaterialIdx)
		{
			const bool bSelected = (MaterialIdx == CurrentMaterial);
			if (ImGui::Selectable(MaterialNames[MaterialIdx].c_str(), bSelected))
			{
				// Null puts it back on the default, which is what entry 0 means
				Primitive.SetMaterial(MaterialIdx == 0 ? nullptr : MaterialPtrs[MaterialIdx]);
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Two sided  %s", Primitive.GetMaterial() && Primitive.GetMaterial()->IsTwoSided() ? "yes" : "no");

	ImGui::PopID();
}

void DebugUI::DrawLightProperties(LightBase& Light)
{
	// Scope of its own, for the same reason the primitive panel has one above
	ImGui::PushID(&Light);

	// Shared by every type
	auto Color = Light.GetColor();
	float ColorRgb[3] = { Color.x, Color.y, Color.z };
	if (ImGui::ColorEdit3("Colour", ColorRgb))
	{
		Light.SetColor({ ColorRgb[0], ColorRgb[1], ColorRgb[2] });
	}

	float Intensity = Light.GetIntensity();
	if (ImGui::DragFloat("Intensity", &Intensity, 0.01f, 0.f, 50.f))
	{
		Light.SetIntensity(Intensity);
	}

	// The rest depends on what kind of light it is
	switch (Light.GetLightType())
	{
	case LightType::Directional:
	{
		auto& Directional = static_cast<DirectionalLight&>(Light);

		// Where the light comes from. Normalized on set, so any non zero vector works.
		auto Direction = Directional.GetDirection();
		float DirectionXyz[3] = { Direction.x, Direction.y, Direction.z };
		if (ImGui::DragFloat3("Direction", DirectionXyz, 0.01f, -1.f, 1.f))
		{
			Directional.SetDirection({ DirectionXyz[0], DirectionXyz[1], DirectionXyz[2] });
		}

		ImGui::TextDisabled("Shadow box is set in code, it has to contain the casters");
		break;
	}

	case LightType::Point:
	{
		auto& Point = static_cast<PointLight&>(Light);

		auto Position = Point.GetPosition();
		float PositionXyz[3] = { Position.x, Position.y, Position.z };
		if (ImGui::DragFloat3("Position", PositionXyz, 0.05f))
		{
			Point.SetPosition({ PositionXyz[0], PositionXyz[1], PositionXyz[2] });
		}

		// How far the light reaches. Also the far plane of its six shadow faces, so
		// moving it changes what ends up in the cube.
		float Range = Point.GetRange();
		if (ImGui::DragFloat("Range", &Range, 0.1f, 0.01f, 500.f))
		{
			Point.SetRange(Range);
		}

		// Size of the emitter. The falloff stops climbing inside it, so a larger radius
		// is a softer, dimmer core and a smaller one a harsher hotspot.
		float SourceRadius = Point.GetSourceRadius();
		if (ImGui::DragFloat("Source radius", &SourceRadius, 0.01f, 0.01f, 10.f))
		{
			Point.SetSourceRadius(SourceRadius);
		}

		ImGui::TextWrapped("Falloff is inverse square, so intensity is energy at the source and not brightness at the surface.");
		break;
	}

	case LightType::Area:
	{
		auto& Area = static_cast<AreaLight&>(Light);

		auto Position = Area.GetPosition();
		float PositionXyz[3] = { Position.x, Position.y, Position.z };
		if (ImGui::DragFloat3("Position", PositionXyz, 0.05f))
		{
			Area.SetPosition({ PositionXyz[0], PositionXyz[1], PositionXyz[2] });
		}

		// Normal of the rectangle, which is the direction it emits towards
		auto Direction = Area.GetDirection();
		float DirectionXyz[3] = { Direction.x, Direction.y, Direction.z };
		if (ImGui::DragFloat3("Direction", DirectionXyz, 0.01f, -1.f, 1.f))
		{
			Area.SetDirection({ DirectionXyz[0], DirectionXyz[1], DirectionXyz[2] });
		}

		// Width and height of the emitter. Only widens the falloff for now, the shadow
		// stays hard because a single view cannot produce a penumbra.
		auto Size = Area.GetSize();
		float SizeXy[2] = { Size.x, Size.y };
		if (ImGui::DragFloat2("Size", SizeXy, 0.05f, 0.01f, 100.f))
		{
			Area.SetSize({ SizeXy[0], SizeXy[1] });
		}

		float Range = Area.GetRange();
		if (ImGui::DragFloat("Range", &Range, 0.1f, 0.01f, 500.f))
		{
			Area.SetRange(Range);
		}
		break;
	}

	default:
		break;
	}

	ImGui::PopID();
}

void DebugUI::DrawSpawnPanel(const AssetCatalog& Catalog, const SIZE_T PrimitiveCount, const SIZE_T LightCount)
{
	ImGui::PushID("SpawnPanel");

	const auto& CatalogModels		= Catalog.GetModels();
	const auto& CatalogMaterials	= Catalog.GetMaterials();

	// Mesh | the built in shapes, then whatever meshes were found on disk
	std::vector<std::string> MeshNames;
	MeshNames.emplace_back("Cube");
	MeshNames.emplace_back("Plane");
	MeshNames.emplace_back("Sphere");

	for (const auto& MeshAsset : CatalogModels)
	{
		char Name[128] = {};
		ToUtf8(MeshAsset.Name, Name, sizeof(Name));
		MeshNames.emplace_back(Name);
	}

	// Material | entry 0 keeps the default, the rest are catalog entries offset by one
	std::vector<std::string> MaterialNames;
	MaterialNames.emplace_back("Default");

	for (const auto& Asset : CatalogMaterials)
	{
		char Name[128] = {};
		ToUtf8(Asset.Name, Name, sizeof(Name));

		// A lone colour map reads differently from a full set of maps, so say which
		MaterialNames.emplace_back(Asset.DiffuseOnlyPath.empty() ? std::string(Name) : std::string(Name) + "  (colour only)");
	}

	SpawnMeshIdx		= SpawnMeshIdx		< 0 ? 0 : (SpawnMeshIdx		>= static_cast<int>(MeshNames.size())		? 0 : SpawnMeshIdx);
	SpawnMaterialIdx	= SpawnMaterialIdx	< 0 ? 0 : (SpawnMaterialIdx	>= static_cast<int>(MaterialNames.size())	? 0 : SpawnMaterialIdx);

	ImGui::SeparatorText("Primitive");

	if (ImGui::BeginCombo("Mesh", MeshNames[static_cast<size_t>(SpawnMeshIdx)].c_str()))
	{
		for (int MeshIdx = 0; MeshIdx < static_cast<int>(MeshNames.size()); ++MeshIdx)
		{
			const bool bSelected = (MeshIdx == SpawnMeshIdx);
			if (ImGui::Selectable(MeshNames[static_cast<size_t>(MeshIdx)].c_str(), bSelected))
			{
				SpawnMeshIdx = MeshIdx;
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Only the sphere is generated rather than loaded, so only it has a density to set.
	// It has to be chosen before the primitive is built; changing it afterwards would
	// mean rebuilding and re-uploading the vertex buffer.
	if (SpawnMeshIdx == SPAWN_MESH_SPHERE)
	{
		int Density = static_cast<int>(SpawnSphereDensity);
		if (ImGui::SliderInt("Density", &Density, static_cast<int>(Sphere::MIN_DENSITY), static_cast<int>(Sphere::MAX_DENSITY)))
		{
			SpawnSphereDensity = static_cast<UINT>(Density);
		}

		ImGui::TextDisabled("%u latitude bands, %u longitude, %u triangles",
							SpawnSphereDensity,
							SpawnSphereDensity * 2,
							Sphere::GetTriangleCountFor(SpawnSphereDensity));
	}

	if (ImGui::BeginCombo("Material", MaterialNames[static_cast<size_t>(SpawnMaterialIdx)].c_str()))
	{
		for (int MaterialIdx = 0; MaterialIdx < static_cast<int>(MaterialNames.size()); ++MaterialIdx)
		{
			const bool bSelected = (MaterialIdx == SpawnMaterialIdx);
			if (ImGui::Selectable(MaterialNames[static_cast<size_t>(MaterialIdx)].c_str(), bSelected))
			{
				SpawnMaterialIdx = MaterialIdx;
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	const bool bPrimitivesFull = PrimitiveCount >= MAX_PRIMITIVES;

	ImGui::BeginDisabled(bPrimitivesFull);
	if (ImGui::Button("Add primitive"))
	{
		PrimitiveSpawnRequest Request;

		switch (SpawnMeshIdx)
		{
		case SPAWN_MESH_CUBE:	Request.Mesh = SpawnMesh::Cube;		break;
		case SPAWN_MESH_PLANE:	Request.Mesh = SpawnMesh::Plane;	break;
		case SPAWN_MESH_SPHERE:	Request.Mesh = SpawnMesh::Sphere;	break;
		default:				Request.Mesh = SpawnMesh::Model;	break;
		}

		// Catalog meshes follow the built in shapes in the list, so the offset comes off
		Request.ModelIdx	= SpawnMeshIdx >= SPAWN_MESH_BUILTIN_COUNT ? SpawnMeshIdx - SPAWN_MESH_BUILTIN_COUNT : -1;
		Request.MaterialIdx	= SpawnMaterialIdx - 1;		// entry 0 is the default, which is -1 to the renderer
		Request.Density		= SpawnSphereDensity;

		PendingPrimitiveSpawns.push_back(Request);
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::TextDisabled("%zu in scene", PrimitiveCount);

	if (bPrimitivesFull)
	{
		ImGui::TextWrapped("The primitive constant buffer is sized for a fixed number of objects and they are all used.");
	}

	ImGui::SeparatorText("Light");

	static const char* LightTypeNames[] = { "Point", "Directional", "Area" };

	ImGui::Combo("Type", &SpawnLightTypeIdx, LightTypeNames, IM_ARRAYSIZE(LightTypeNames));

	const bool bLightsFull = LightCount >= MAX_LIGHTS;

	ImGui::BeginDisabled(bLightsFull);
	if (ImGui::Button("Add light"))
	{
		LightSpawnRequest Request;
		switch (SpawnLightTypeIdx)
		{
		case 1:		Request.Type = LightType::Directional;	break;
		case 2:		Request.Type = LightType::Area;			break;
		default:	Request.Type = LightType::Point;		break;
		}

		PendingLightSpawns.push_back(Request);
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::TextDisabled("%zu of %u", LightCount, MAX_LIGHTS);

	if (bLightsFull)
	{
		ImGui::TextWrapped("Every light owns a slice of the shadow atlas, and the atlas is built for MAX_LIGHTS of them.");
	}

	ImGui::TextDisabled("New objects appear five units in front of the camera.");

	ImGui::PopID();
}

void DebugUI::Draw(Camera& Camera,
					const AssetCatalog& Catalog,
					std::vector<std::unique_ptr<Primitive>>& Primitives,
					std::vector<std::unique_ptr<LightBase>>& Lights,
					const std::vector<std::unique_ptr<Material>>& Materials,
					Material * const DefaultMaterial,
					const UINT FramesPerSecond)
{
	if (!IsInitialized())
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(420.f, 560.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(16.f, 16.f), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("DXI Debug"))
	{
		ImGui::End();
		return;
	}

	// 5 | Frame rate. ImGui keeps its own running average, the engine counter comes from
	// StepTimer, so both are shown rather than picking one to trust.
	if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto& IO = ImGui::GetIO();
		ImGui::Text("Engine   %u fps", FramesPerSecond);
		ImGui::Text("ImGui    %.1f fps  (%.3f ms/frame)", IO.Framerate, IO.Framerate > 0.f ? 1000.f / IO.Framerate : 0.f);
	}

	// Surface debug views. BasePass writes these into the albedo target instead of the
	// material colour, and flags them so LightPass passes them through unshaded.
	if (ImGui::CollapsingHeader("Base Pass Derivatives", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static const char* ModeNames[] =
		{
			"Off",
			"UV density (length of ddx and ddy)",
			"ddx(uv) raw, signed",
			"ddy(uv) raw, signed",
			"Mip level (texel density)",
			"World derivatives (fwidth of position)",
			"Normal derivatives (fwidth of normal)"
		};
		static_assert(IM_ARRAYSIZE(ModeNames) == static_cast<int>(BasePassDebugMode::Count),
						"Every BasePassDebugMode needs a label in the panel");

		int CurrentMode = static_cast<int>(BasePassDebug);
		if (ImGui::Combo("##BasePassDebug", &CurrentMode, ModeNames, IM_ARRAYSIZE(ModeNames)))
		{
			BasePassDebug = static_cast<BasePassDebugMode>(CurrentMode);
		}

		// What the pixels mean, the values themselves are in the G-buffer for a capture
		switch (BasePassDebug)
		{
		case BasePassDebugMode::UVDensity:
			ImGui::TextWrapped("Red is how far the uv travels per pixel horizontally, green vertically. Bright means a lot of texture in few pixels, a red or green tint means it is stretched along that axis.");
			break;
		case BasePassDebugMode::UVDerivativeDdx:
		case BasePassDebugMode::UVDerivativeDdy:
			ImGui::TextWrapped("The derivative itself, u in red and v in green. Mid grey is zero, brighter is positive and darker is negative. Four numbers do not fit in three channels, so ddx and ddy get a view each.");
			break;
		case BasePassDebugMode::MipLevel:
			ImGui::TextWrapped("Black is mip 0, brighter is a smaller mip. Blue means magnified past mip 0, where the texture is stretched.");
			break;
		case BasePassDebugMode::WorldDerivatives:
			ImGui::TextWrapped("World units covered by one pixel. Surfaces seen edge on change fastest and light up.");
			break;
		case BasePassDebugMode::NormalDerivatives:
			ImGui::TextWrapped("Every mesh here still carries one normal per face, so this is black inside a triangle and only shows up where two of them meet. A mesh with real per vertex normals would show a gradient.");
			break;
		default:
			ImGui::TextDisabled("Materials rendered normally");
			break;
		}
	}

	// 2 | Camera
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto& Position = Camera.GetPosition();
		ImGui::Text("Position  x %8.3f  y %8.3f  z %8.3f", Position.x, Position.y, Position.z);

		// Degrees, which is the unit the camera stores them in. Pitch stops short of
		// straight up, otherwise the up vector and the direction line up and the view
		// matrix has no way left to tell which way is up.
		ImGui::Text("Rotation  yaw %7.2f  pitch %7.2f", Camera.GetYaw(), Camera.GetPitch());

		// The basis the view matrix is built from, rebuilt out of yaw and pitch each frame
		const auto& Forward	= Camera.GetDirection();
		const auto& Up		= Camera.GetUp();
		const auto& Right	= Camera.GetRight();
		ImGui::Text("Forward   x %8.3f  y %8.3f  z %8.3f", Forward.x, Forward.y, Forward.z);
		ImGui::Text("Up        x %8.3f  y %8.3f  z %8.3f", Up.x, Up.y, Up.z);
		ImGui::Text("Right     x %8.3f  y %8.3f  z %8.3f", Right.x, Right.y, Right.z);

		float Sensitivity = Camera.GetMouseSensitivity();
		if (ImGui::DragFloat("Mouse sensitivity", &Sensitivity, 0.01f, 0.01f, 2.0f, "%.2f deg/px"))
		{
			Camera.SetMouseSensitivity(Sensitivity);
		}
	}

	// 1 | Projection matrix
	if (ImGui::CollapsingHeader("Projection Matrix", ImGuiTreeNodeFlags_DefaultOpen))
	{
		XMFLOAT4X4 ProjMat;
		XMStoreFloat4x4(&ProjMat, Camera.GetProjMatrix());
		DrawMatrix(ProjMat);
	}

	// The other half of what turns a world position into clip space, handy next to the
	// NDC readout further down when a vertex lands somewhere unexpected
	if (ImGui::CollapsingHeader("View Matrix"))
	{
		XMFLOAT4X4 ViewMat;
		XMStoreFloat4x4(&ViewMat, Camera.GetViewMatrix());
		DrawMatrix(ViewMat);
	}

	// Everything the scene can be built out of, discovered on disk rather than listed in
	// code. The renderer picks these requests up at the top of its next frame.
	if (ImGui::CollapsingHeader("Add to Scene", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawSpawnPanel(Catalog, Primitives.size(), Lights.size());
	}

	// 3 and 4 | Pick an object out of the scene and look at where it is, in world space
	// and all the way through to normalized device coordinates
	if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Primitives.empty())
		{
			ImGui::TextDisabled("No primitives in the scene");
		}
		else
		{
			// Keep the selection inside the list, the scene can change between frames
			SelectedPrimitiveIdx = SelectedPrimitiveIdx < 0 ? 0 : SelectedPrimitiveIdx;
			SelectedPrimitiveIdx = SelectedPrimitiveIdx >= static_cast<int>(Primitives.size())
									? static_cast<int>(Primitives.size()) - 1
									: SelectedPrimitiveIdx;

			if (ImGui::BeginListBox("##Primitives", ImVec2(-FLT_MIN, 5.f * ImGui::GetTextLineHeightWithSpacing())))
			{
				for (int PrimitiveIdx = 0; PrimitiveIdx < static_cast<int>(Primitives.size()); ++PrimitiveIdx)
				{
					char Name[128] = {};
					ToUtf8(Primitives[PrimitiveIdx]->Name, Name, sizeof(Name));

					const bool bSelected = (PrimitiveIdx == SelectedPrimitiveIdx);
					if (ImGui::Selectable(Name, bSelected))
					{
						SelectedPrimitiveIdx = PrimitiveIdx;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}

			auto& Selected = *Primitives[SelectedPrimitiveIdx];

			ImGui::SeparatorText("Properties");
			DrawPrimitiveProperties(Selected, Materials, DefaultMaterial);

			ImGui::SeparatorText("Location");
			const auto& WorldPosition = Selected.GetPosVector();
			ImGui::Text("World     x %8.3f  y %8.3f  z %8.3f", WorldPosition.x, WorldPosition.y, WorldPosition.z);

			// Same point pushed through the camera. Clip space is what the vertex shader
			// writes, dividing it by w is what turns it into NDC.
			const auto ViewProj	= Camera.GetViewMatrix() * Camera.GetProjMatrix();
			const auto Clip		= XMVector4Transform(XMVectorSet(WorldPosition.x, WorldPosition.y, WorldPosition.z, 1.f), ViewProj);

			XMFLOAT4 ClipF;
			XMStoreFloat4(&ClipF, Clip);

			ImGui::SeparatorText("Clip space");
			ImGui::Text("x %8.3f  y %8.3f  z %8.3f  w %8.3f", ClipF.x, ClipF.y, ClipF.z, ClipF.w);

			ImGui::SeparatorText("NDC");
			if (fabsf(ClipF.w) < 1e-6f)
			{
				ImGui::TextDisabled("w is zero, the point sits on the camera plane");
			}
			else
			{
				const float InvW = 1.f / ClipF.w;
				const XMFLOAT4 Ndc(ClipF.x * InvW, ClipF.y * InvW, ClipF.z * InvW, 1.f);

				ImGui::Text("x %8.3f  y %8.3f  z %8.3f  w %8.3f", Ndc.x, Ndc.y, Ndc.z, Ndc.w);

				// Behind the camera shows up as a negative w, which is why that is checked
				// separately from the box test
				const bool bInside = ClipF.w > 0.f
									&& fabsf(Ndc.x) <= 1.f
									&& fabsf(Ndc.y) <= 1.f
									&& Ndc.z >= 0.f && Ndc.z <= 1.f;
				ImGui::Text("On screen  %s", bInside ? "yes" : "no");
			}
		}
	}

	// Lights. Every one of them casts a shadow now, so moving a light or changing its
	// range re-aims its shadow views on the next frame too.
	if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Lights.empty())
		{
			ImGui::TextDisabled("No lights in the scene");
		}
		else
		{
			SelectedLightIdx = SelectedLightIdx < 0 ? 0 : SelectedLightIdx;
			SelectedLightIdx = SelectedLightIdx >= static_cast<int>(Lights.size())
								? static_cast<int>(Lights.size()) - 1
								: SelectedLightIdx;

			if (ImGui::BeginListBox("##Lights", ImVec2(-FLT_MIN, 4.f * ImGui::GetTextLineHeightWithSpacing())))
			{
				for (int LightIdx = 0; LightIdx < static_cast<int>(Lights.size()); ++LightIdx)
				{
					char Name[160] = {};
					char Label[200] = {};
					ToUtf8(Lights[LightIdx]->GetName(), Name, sizeof(Name));

					// The type matters for what the properties below will be
					char TypeName[64] = {};
					ToUtf8(ToString(Lights[LightIdx]->GetLightType()), TypeName, sizeof(TypeName));
					sprintf_s(Label, "%s  (%s)", Name, TypeName);

					const bool bSelected = (LightIdx == SelectedLightIdx);
					if (ImGui::Selectable(Label, bSelected))
					{
						SelectedLightIdx = LightIdx;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}

			ImGui::SeparatorText("Properties");
			DrawLightProperties(*Lights[SelectedLightIdx]);
		}
	}

	ImGui::End();
}

void DebugUI::Render(DXGraphicsCommandList * CommandList)
{
	if (!IsInitialized())
	{
		return;
	}

	DXASSERT(CommandList, "Command list can not be null for rendering the debug UI");

	ImGui::Render();

	// The font lives in this heap, so it has to be the bound one while the UI draws
	DXDescriptorHeap* Heaps[] = { FontHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);
}

bool DebugUI::HandleWindowMessage(const HWND WindowHandle, const UINT Message, const WPARAM WParam, const LPARAM LParam)
{
	if (ImGui::GetCurrentContext() == nullptr)
	{
		return false;
	}

	return ImGui_ImplWin32_WndProcHandler(WindowHandle, Message, WParam, LParam) != 0;
}

bool DebugUI::WantsMouse()
{
	return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
}

bool DebugUI::WantsKeyboard()
{
	return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
}
