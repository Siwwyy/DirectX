//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "D3D12Renderer.h"

// Own Includes
// Scene building lives in D3D12RendererSceneInit.cpp, which is where Paths.h and
// GltfLoader.h went with it. Nothing of Win32Proc in here: the window is the host's.
#include "CommandLine.h"
#include "Model.h"

#include <string>
#include "PixEvents.h"
#include "Paths.h"

// Namespaces
using namespace Helpers;


D3D12Renderer::D3D12Renderer(UINT WindowWidth, UINT WindowHeight, std::wstring WindowName)
	: WindowWidth(WindowWidth)
	, WindowHeight(WindowHeight)
	, AspectRatio(static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight))
	, WindowName(std::move(WindowName))
	, ViewPort(0.f, 0.f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, 1.0f)
	, ScissorRect(0, 0, static_cast<LONG>(WindowWidth), static_cast<LONG>(WindowHeight))
	, Camera(WindowWidth, WindowHeight)
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");
}

void D3D12Renderer::Initialize()
{
	DXASSERT(WindowHandle, "SetWindowHandle has to be called before Initialize, the swap chain presents into that window");

	// Core | Device first, everything else is built on top of it.
	// Every one of these is a debugging knob, so it comes off the command line.
	DXIDeviceOptions DeviceOptions;
	DeviceOptions.bEnableGpuValidation	= DXICMD::HasFlag(L"gpuvalidation");
	DeviceOptions.bEnableDebugLayer		= !DXICMD::HasFlag(L"nodebuglayer");
	DeviceOptions.bBreakOnSeverity		= !DXICMD::HasFlag(L"nobreak");
	DeviceOptions.bUseWarpAdapter		= DXICMD::HasFlag(L"warp");

	Device.Init(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, DeviceOptions);

	// Settled before anything is built, because the material and light setup both ask
	// which scene is coming
	ActiveScene = GetRequestedScene();

	// Off by default, since a pinned frame rate says nothing about how long a frame takes
	bVSyncEnabled = DXICMD::HasFlag(L"vsync");
	SwapChain.Init(Device, WindowHandle, WindowWidth, WindowHeight, BACK_BUFFER_COUNT);
	DepthBuffer.Init(Device, WindowWidth, WindowHeight);
	FrameContext.Init(Device, BACK_BUFFER_COUNT);

	CurrentFrameIdx = SwapChain.GetCurrentBackBufferIdx();

	// Scene records uploads into the command list, so it has to come before the flush below
	InitializeScene();
	InitializeConstantBuffers();
	InitializePasses();

	// Debug panel. It uploads a font atlas of its own, so it goes in before the flush. A host
	// with panels of its own leaves it out altogether, see SetDebugUIEnabled.
	if (bDebugUIEnabled)
	{
		SceneDebugUI.Init(Device, WindowHandle, DXISwapChain::GetFormat(), BACK_BUFFER_COUNT);
	}

	// Shadows are off unless asked for. The panel owns the switch so it can be flipped
	// while running; this only decides what it starts as.
	SceneDebugUI.SetShadowsEnabled(DXICMD::HasFlag(L"shadows"));

	// Push the vertex, index and texture uploads to the GPU and wait them out
	FrameContext.Flush(Device, CurrentFrameIdx);

	// Past the flush every copy recorded above has completed, so the staging copies of the
	// textures are no longer needed. Worth a few hundred megabytes once a scene the size of
	// Sponza is loaded, see Material::ReleaseUploadHeaps.
	//
	// Materials created later, by the spawn panel, keep theirs: they are uploaded on a
	// frame's command list and nothing waits on it here.
	for (auto& LoadedMaterial : Materials)
	{
		LoadedMaterial->ReleaseUploadHeaps();
	}

	DefaultMaterial->ReleaseUploadHeaps();

	bInitialized = true;
}

void D3D12Renderer::Resize(const UINT NewWidth, const UINT NewHeight)
{
	// Windows sends a WM_SIZE while the window is still being created, and a host can do the same.
	// There is nothing to resize yet, and what gets built is the size the constructor was given.
	if (!bInitialized || bDestroyed)
	{
		return;
	}

	// A minimized window is 0 by 0, and neither the swap chain nor any texture can be that. Keep
	// everything as it is and stop rendering, the restore brings the old size right back.
	if (NewWidth == 0 || NewHeight == 0)
	{
		bMinimized = true;
		return;
	}

	bMinimized = false;

	if (NewWidth == WindowWidth && NewHeight == WindowHeight)
	{
		return;
	}

	// Frames are no longer waited on after Present, so up to BACK_BUFFER_COUNT of them can still
	// be reading the buffers about to be replaced. The command list is closed between frames.
	FrameContext.WaitForGpuIdle(Device);

	// Everything that is as big as the screen. The shadow atlas is not, and is left alone.
	SwapChain.Resize(Device, NewWidth, NewHeight);
	DepthBuffer.Init(Device, NewWidth, NewHeight);
	SceneGBuffer.Init(Device.Get(), NewWidth, NewHeight);

	// The old resources are gone, the light pass would sample freed memory through its old views
	WriteLightInputViews();

	WindowWidth		= NewWidth;
	WindowHeight	= NewHeight;
	AspectRatio		= static_cast<float>(NewWidth) / static_cast<float>(NewHeight);
	ViewPort		= CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(NewWidth), static_cast<float>(NewHeight), 0.0f, 1.0f);
	ScissorRect		= CD3DX12_RECT(0, 0, static_cast<LONG>(NewWidth), static_cast<LONG>(NewHeight));

	Camera.SetScreenSize(NewWidth, NewHeight);

	// The chain may have handed a different back buffer out first after the resize
	CurrentFrameIdx = SwapChain.GetCurrentBackBufferIdx();
}

void D3D12Renderer::ProcessPendingSpawns()
{
	auto& PrimitiveRequests	= SceneDebugUI.GetPendingPrimitiveSpawns();
	auto& LightRequests		= SceneDebugUI.GetPendingLightSpawns();

	if (PrimitiveRequests.empty() && LightRequests.empty())
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	for (const auto& Request : PrimitiveRequests)
	{
		if (Primitives.size() >= MAX_PRIMITIVES)
		{
			DXLOG("Primitive constant buffer is full, cannot spawn another primitive\n")
			break;
		}

		// A name that says what it is and stays unique, since the outliner lists by name
		const auto Ordinal = std::to_wstring(Primitives.size());

		std::unique_ptr<Primitive> Spawned;
		switch (Request.Mesh)
		{
		case SpawnMesh::Plane:
			Spawned = std::make_unique<::Plane>(L"Plane " + Ordinal);
			break;

		case SpawnMesh::Sphere:
			Spawned = std::make_unique<Sphere>(L"Sphere " + Ordinal, Request.Density);
			break;

		case SpawnMesh::Model:
		{
			const auto& CatalogModels = Catalog.GetModels();
			if (Request.ModelIdx < 0 || Request.ModelIdx >= static_cast<int>(CatalogModels.size()))
			{
				continue;
			}

			const auto& Asset = CatalogModels[static_cast<size_t>(Request.ModelIdx)];
			Spawned = std::make_unique<Model>(Asset.Name + L" " + Ordinal, Asset.MeshPath);
			break;
		}

		default:
			Spawned = std::make_unique<Cube>(L"Cube " + Ordinal);
			break;
		}

		// Records the vertex and index uploads into the open command list
		Spawned->Init(Device.Get(), CommandList);

		// Null leaves it on the default material, which is what MaterialIdx -1 means
		Spawned->SetMaterial(EnsureMaterialLoaded(Request.MaterialIdx));

		// Dropped in front of the camera rather than at the origin, where it would be
		// inside the ground plane and easy to miss
		const auto& ViewPosition	= Camera.GetPosition();
		const auto& Forward			= Camera.GetDirection();
		Spawned->SetTransform({ ViewPosition.x + (Forward.x * 5.f),
								ViewPosition.y + (Forward.y * 5.f),
								ViewPosition.z + (Forward.z * 5.f) });

		// Previous frame starts equal to this one, so something that has just appeared
		// reports no motion. This runs after the transform on purpose: seeding before it
		// would have the object appear to fly in from wherever it was spawned.
		Spawned->AdvanceFrame();

		Primitives.emplace_back(std::move(Spawned));
	}

	for (const auto& Request : LightRequests)
	{
		if (Lights.size() >= MAX_LIGHTS)
		{
			DXLOG("Shadow atlas has a slice per light and they are all taken\n")
			break;
		}

		const auto Ordinal		= std::to_wstring(Lights.size());
		const auto& ViewPosition	= Camera.GetPosition();
		const auto& ViewForward		= Camera.GetDirection();

		switch (Request.Type)
		{
		case LightType::Directional:
		{
			auto Spawned = std::make_unique<DirectionalLight>(L"Directional " + Ordinal);
			Spawned->SetDirection({ -0.5f, -1.0f, 0.5f });
			Spawned->SetIntensity(0.6f);
			AddLight(std::move(Spawned));
			break;
		}

		case LightType::Area:
		{
			auto Spawned = std::make_unique<AreaLight>(L"Area " + Ordinal);
			Spawned->SetPosition({ ViewPosition.x, ViewPosition.y, ViewPosition.z });
			Spawned->SetDirection({ 0.0f, -1.0f, 0.0f });
			Spawned->SetIntensity(12.0f);
			AddLight(std::move(Spawned));
			break;
		}

		case LightType::Spot:
		{
			auto Spawned = std::make_unique<SpotLight>(L"Spot " + Ordinal);
			Spawned->SetPosition({ ViewPosition.x, ViewPosition.y, ViewPosition.z });

			// Pointed where the camera is looking, so a spawned spot lights what is on screen
			Spawned->SetDirection({ ViewForward.x, ViewForward.y, ViewForward.z });
			Spawned->SetIntensity(12.0f);
			AddLight(std::move(Spawned));
			break;
		}

		default:
		{
			auto Spawned = std::make_unique<PointLight>(L"Point " + Ordinal);
			Spawned->SetPosition({ ViewPosition.x, ViewPosition.y, ViewPosition.z });
			Spawned->SetIntensity(6.0f);
			Spawned->SetSourceRadius(0.2f);
			AddLight(std::move(Spawned));
			break;
		}
		}
	}

	PrimitiveRequests.clear();
	LightRequests.clear();
}

void D3D12Renderer::InitializeConstantBuffers()
{
	DXASSERT(!Primitives.empty(), "Scene must be built before its constant buffers are sized");

	// Every buffer holds one region per frame in flight, so the GPU never reads what the CPU is writing
	const auto CreateMappedBuffer = [this](const UINT64 SizeInBytes, ComPtr<DXResource>& OutBuffer, UINT8** OutAddress, LPCWSTR Name)
	{
		// Rounded here rather than at every call site, so no buffer can be created at a
		// size a constant buffer view cannot span
		const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignConstantBufferSize(SizeInBytes));
		ThrowIfFailed(Device.Get()->CreateCommittedResource(
			&DX_HEAP_PROPERTY_UPLOAD,			// this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE,				// no flags
			&BufferDesc,						// size of the resource heap
			D3D12_RESOURCE_STATE_GENERIC_READ,	// will be data that is read from so we keep it in the generic read state
			nullptr,							// we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&OutBuffer)));
		OutBuffer->SetName(Name);

		// We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
		CD3DX12_RANGE ReadRange(0, 0);
		ThrowIfFailed(OutBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(OutAddress)));
	};

	// Sized for the capacity rather than for what the scene happens to hold right now,
	// because the panel can add primitives after this has been created and mapped
	// Strides rather than sizes, because every offset taken into these has to be something
	// a constant buffer view can start at
	const UINT64 PrimitiveRegionSize = static_cast<UINT64>(MAX_PRIMITIVES) * PrimitiveCbvStride;

	// Create mapped buffers
	CreateMappedBuffer(PrimitiveRegionSize * BACK_BUFFER_COUNT,			PrimitiveCBV,	&PrimitiveCbvAddress,	L"Primitive Constant Buffer");
	CreateMappedBuffer(CameraCbvStride * BACK_BUFFER_COUNT,				CameraCBV,		&CameraCbvAddress,		L"Camera Constant Buffer");
	CreateMappedBuffer(LightsCbvStride * BACK_BUFFER_COUNT,				LightsCBV,		&LightsCbvAddress,		L"Lights Constant Buffer");

	// Seeds the previous transforms with the current ones before anything is uploaded, so
	// the opening frame compares the scene against where it already is and reports no
	// motion. Without this the first frame would measure against the identity matrix.
	VelocityPass();

	// Fill the first frame so nothing reads uninitialized memory before Update() runs
	UploadCameraConstants();
	UploadLightConstants();
	UploadPrimitiveConstants();
}

void D3D12Renderer::InitializePasses()
{
	// ShadowPass writes the depth of the scene from the light, BasePass draws the
	// primitives with their materials into the G-buffer, LightPass shades the result
	SceneGBuffer.Init(Device.Get(), WindowWidth, WindowHeight);
	SceneShadowAtlas.Init(Device.Get(), SHADOW_MAP_RESOLUTION, SHADOW_CUBE_RESOLUTION);

	SceneShadowPass.Init(Device.Get(), FrameContext.GetCommandList());
	SceneBasePass.Init(Device.Get(), FrameContext.GetCommandList());
	SceneLightPass.Init(Device.Get(), FrameContext.GetCommandList());
	SceneTangentFramePass.Init(Device.Get(), FrameContext.GetCommandList());
	SceneAxisGizmoPass.Init(Device.Get(), FrameContext.GetCommandList());
	SceneSelectionPass.Init(Device.Get(), FrameContext.GetCommandList());
	SceneTransformGizmoPass.Init(Device.Get(), FrameContext.GetCommandList());

	// Only one CBV/SRV/UAV heap can be bound at a time, so everything LightPass samples
	// goes into a single heap: the G-buffer targets, the shadow map, then both sky cubes
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc	= {};
	HeapDesc.NumDescriptors				= LightPass::GetSrvCount();
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(Device.Get()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&LightInputsHeap)));
	NAME_D3D12_OBJECT(LightInputsHeap, L"Light Pass Inputs Heap");

	WriteLightInputViews();
}

void D3D12Renderer::WriteLightInputViews()
{
	const auto IncrementSize = Device.Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHandle(LightInputsHeap->GetCPUDescriptorHandleForHeapStart());

	SceneGBuffer.CreateShaderResourceViews(Device.Get(), SrvHandle, IncrementSize);

	// Depth sits immediately after the colour targets, so the pixel shader reads the whole
	// surface description as one run of registers.
	//
	// Anything that recreates the depth buffer has to rewrite this view as well, exactly as
	// it does for the G-buffer's. The view outlives the resource it points at otherwise, and
	// the symptom is a frame sampling freed memory rather than a crash. Resize is that, and
	// calls this for it.
	SrvHandle.Offset(GBufferTargetCount, IncrementSize);
	DepthBuffer.CreateShaderResourceView(Device, SrvHandle);

	SrvHandle.Offset(GBUFFER_DEPTH_SRV_COUNT, IncrementSize);
	SceneShadowAtlas.CreateShaderResourceViews(Device.Get(), SrvHandle, IncrementSize);

// The sky cubes used to follow the shadow atlas here, at t7 and t8
}

XMMATRIX D3D12Renderer::GetLightShadowViewProj(const LightBase& Light, const UINT FaceIdx) const
{
	// Each type aims its shadow camera differently, and only the point light has more
	// than one view to aim
	switch (Light.GetLightType())
	{
	case LightType::Directional:
		return static_cast<const DirectionalLight&>(Light).GetShadowViewProj();

	case LightType::Point:
		return static_cast<const PointLight&>(Light).GetShadowViewProj(FaceIdx);

	case LightType::Area:
		return static_cast<const AreaLight&>(Light).GetShadowViewProj();

	case LightType::Spot:
		return static_cast<const SpotLight&>(Light).GetShadowViewProj();

	default:
		return XMMatrixIdentity();
	}
}

void D3D12Renderer::Update(float DeltaTime)
{
	if (bDestroyed || !bInitialized)
	{
		return;
	}

	// Win32Proc hands us a zero delta, so the timer is the real source of elapsed time
	StepTimer.Tick(NULL);
	const auto ElapsedSeconds = static_cast<float>(StepTimer.GetElapsedSeconds());

	// Update window text with FPS value. Not a host's business: its window has a title of its own.
	if (bStatsInWindowTitle && WindowHandle && (CurrentFrameIdx % 509))
	{
		wchar_t Fps[64];
		swprintf_s(Fps, L"%ufps", StepTimer.GetFramesPerSecond());

		// Saying which it is makes a frame rate pinned to the refresh rate obvious
		const std::wstring VSyncText	= bVSyncEnabled ? L" | VSync On" : L" | VSync Off";
		const std::wstring WindowText	= WindowName + L" | Deferred" + VSyncText + L" | FPS: " + Fps;
		SetWindowText(WindowHandle, WindowText.c_str());
	}

	// Camera matrices
	Camera.Update(ElapsedSeconds);

	// NOTE | The constant uploads deliberately do not happen here. Update() runs before
	// BeginFrame() picks the frame index, so writing them now would target the region the
// GPU may still be reading. Render() uploads them once the index is known.
}

void D3D12Renderer::Render()
{
	// Nothing to draw into while the window is minimized, and nothing yet before Initialize
	if (bDestroyed || !bInitialized || bMinimized)
	{
		return;
	}

	//Always BeginFrame first
	BeginFrame();

	// Whatever the panel asked for last frame. Has to come before the constant uploads
	// below, so anything spawned here gets its matrices written on the same frame it
	// first draws, and after BeginFrame, because spawning records copy commands.
	ProcessPendingSpawns();

	// Now that the frame index is settled and the GPU is known to be done with that
	// frame's region, its constants can be written
	UploadCameraConstants();
	UploadLightConstants();
	UploadPrimitiveConstants();

	// Every upload above has read the previous transforms, so they can be replaced now.
	// Doing it before the uploads would make the velocity target read zero every frame,
	// which is the quiet kind of wrong: the scene still renders, it just never moves.
	VelocityPass();

	// Half of it. UploadLightConstants hands out NO_SHADOW_CASTER so nothing reads the atlas
	if (SceneDebugUI.AreShadowsEnabled())
	{
		RenderShadowPass();
	}

	RenderBasePass();
	RenderLightPass();

	// Right here the frame is shaded and nothing is drawn over it yet, no overlays and no UI,
	// which is the picture the buffers describe
	CaptureFrameDump();

	RenderTangentFramePass();
	RenderAxisGizmoPass();

	// The selection goes over the other overlays, and its handles over the selection: the handles
	// are what the mouse is working with, so nothing may hide them
	RenderSelectionPass();
	RenderTransformGizmoPass();
	RenderDebugUI();

	//Always EndFrame last
	EndFrame();
}

void D3D12Renderer::BeginFrame()
{
	// Pick the frame first, then reset the resources belonging to it
	CurrentFrameIdx = SwapChain.GetCurrentBackBufferIdx();
	FrameContext.BeginFrame(Device, CurrentFrameIdx);

	auto * const CommandList = FrameContext.GetCommandList();

	// Indicate that the back buffer will be used as a render target
	SwapChain.TransitionToRenderTarget(CommandList, CurrentFrameIdx);

	// Set necessary state.
	// NOTE | Each pass binds its own root signature, pipeline state and render targets
	CommandList->RSSetViewports(1, &ViewPort);
	CommandList->RSSetScissorRects(1, &ScissorRect);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// The previous frame's light pass left this readable, and a clear needs it writable
	DepthBuffer.TransitionToDepthWrite(CommandList);
	DepthBuffer.Clear(CommandList);

	// Opens the UI frame, the widgets are filled in once the passes are done
	if (UsesDebugUI())
	{
		SceneDebugUI.BeginFrame();
	}
}

void D3D12Renderer::RenderShadowPass()
{
	if (Lights.empty())
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	// Only as many lights as the atlas has slices for, which is also what the loop below
	// walks. Hoisted above the marker so the event can name it.
	const auto CasterCount = Lights.size() < MAX_LIGHTS ? Lights.size() : MAX_LIGHTS;

	// The scene from every light in turn. A directional or area light is one depth view,
	// a point light is six faces of a cube, so this is up to MAX_LIGHTS * 6 draws of the
	// whole scene per frame.
	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::SHADOW_PASS, L"Shadow Pass | %u casters", static_cast<UINT>(CasterCount));
	{
		SceneShadowAtlas.TransitionToWrite(CommandList);

		// Both pipelines share the root signature, only the pipeline state changes
		CommandList->SetGraphicsRootSignature(SceneShadowPass.GetRootSignature());

		// Records one view: aim the shadow camera, then draw the scene into it
		const auto RenderView = [&](const LightBase& Light, const UINT FaceIdx)
		{
			ShadowViewConstants ViewConstants = {};
			XMStoreFloat4x4(&ViewConstants.WorldToLightClip, XMMatrixTranspose(GetLightShadowViewProj(Light, FaceIdx)));

			// Only a cube face reads these, it needs them to store a normalized distance
			if (Light.GetLightType() == LightType::Point)
			{
				const auto& PointLightRef		= static_cast<const PointLight&>(Light);
				ViewConstants.LightPosition		= PointLightRef.GetPosition();
				ViewConstants.LightRange		= PointLightRef.GetRange();
			}

			CommandList->SetGraphicsRoot32BitConstants(SceneShadowPass.ShadowViewIdx, ShadowViewConstantCount, &ViewConstants, 0);

			for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
			{
				CommandList->SetGraphicsRootConstantBufferView(SceneShadowPass.PrimitiveCBVIdx, GetPrimitiveCbvAddress(PrimitiveIdx));
				SceneShadowPass.RenderPrimitive(CommandList, *Primitives[PrimitiveIdx]);
			}
		};

		for (UINT LightIdx = 0; LightIdx < static_cast<UINT>(CasterCount); ++LightIdx)
		{
			const auto& Light = *Lights[LightIdx];

			DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::VIEW, L"%s | %s | slot %u",
							Light.GetName().c_str(),
							ToString(Light.GetLightType()),
							LightIdx);
			{
				if (Light.GetLightType() == LightType::Point)
				{
					// Distance into a colour target, one face at a time
					CommandList->SetPipelineState(SceneShadowPass.GetDistancePipelineState());

					// Same order as PointLight::GetShadowViewProj and the cube sampler, so
					// a capture names the face rather than leaving six identical events
					static const wchar_t* const FaceNames[ShadowAtlas::CUBE_FACE_COUNT] =
					{
						L"+X", L"-X", L"+Y", L"-Y", L"+Z", L"-Z"
					};

					for (UINT FaceIdx = 0; FaceIdx < ShadowAtlas::CUBE_FACE_COUNT; ++FaceIdx)
					{
						DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::VIEW, L"Face %s", FaceNames[FaceIdx]);
						{
							SceneShadowAtlas.BindCubeFace(CommandList, LightIdx, FaceIdx);
							RenderView(Light, FaceIdx);
						}
						DXI_PIX_END(CommandList);
					}
				}
				else
				{
					// Depth only, a single view is enough
					CommandList->SetPipelineState(SceneShadowPass.GetPipelineState());

					SceneShadowAtlas.BindDepthSlice(CommandList, LightIdx);
					RenderView(Light, 0);
				}
			}
			DXI_PIX_END(CommandList);
		}

		// Back to the screen sized viewport for the rest of the frame
		CommandList->RSSetViewports(1, &ViewPort);
		CommandList->RSSetScissorRects(1, &ScissorRect);
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::RenderBasePass()
{
	auto * const CommandList = FrameContext.GetCommandList();

	// Surface only, every primitive lands in the G-buffer with its material applied and no lighting
	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::BASE_PASS, L"Base Pass | %zu primitives", Primitives.size());
	{
		SceneGBuffer.TransitionToRenderTarget(CommandList);
		DepthBuffer.TransitionToDepthWrite(CommandList);
		SceneGBuffer.BindAsRenderTarget(CommandList, DepthBuffer.GetDsvHandle());
		SceneGBuffer.Clear(CommandList);

		CommandList->SetGraphicsRootSignature(SceneBasePass.GetRootSignature());

		// set the descriptor heap shared by every material
		DXDescriptorHeap* MaterialHeaps[] = { SharedMaterialHeap.GetHeap() };
		CommandList->SetDescriptorHeaps(_countof(MaterialHeaps), MaterialHeaps);

		// Which debug view the panel asked for, the same for every primitive
		CommandList->SetGraphicsRoot32BitConstant(SceneBasePass.DebugModeIdx, static_cast<UINT>(SceneDebugUI.GetBasePassDebugMode()), 0);

		// Both variants share the root signature, only the pipeline differs, so the
		// state is swapped lazily instead of once per draw
		DXPipelineState* BoundPipeline = nullptr;

		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			const auto& Primitive = *Primitives[PrimitiveIdx];

			// Either the primitive's own material or the default it falls back to, so the
			// table is rebound per draw
			const auto * const PrimitiveMaterial = Primitive.GetMaterial();
			DXASSERT(PrimitiveMaterial, "No material and no default registered, call Primitive::SetDefaultMaterial first");

			auto * const Pipeline = SceneBasePass.GetPipelineStateFor(PrimitiveMaterial->IsTwoSided());
			if (Pipeline != BoundPipeline)
			{
				CommandList->SetPipelineState(Pipeline);
				BoundPipeline = Pipeline;
			}

			CommandList->SetGraphicsRootDescriptorTable(SceneBasePass.MaterialTableIdx, PrimitiveMaterial->GetDescriptorTable());
			CommandList->SetGraphicsRootConstantBufferView(SceneBasePass.PrimitiveCBVIdx, GetPrimitiveCbvAddress(PrimitiveIdx));
			SceneBasePass.RenderPrimitive(CommandList, Primitive);
		}
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::RenderLightPass()
{
	auto * const CommandList = FrameContext.GetCommandList();

	// Shades what BasePass wrote, straight into the back buffer.
	// Depth comes along with the G-buffer now: the base pass rendered through it, so it has
	// to be flipped to a readable state before this pass samples the same memory.
	SceneGBuffer.TransitionToShaderResource(CommandList);
	DepthBuffer.TransitionToShaderResource(CommandList);

	// Only when the pass ran. ShadowAtlas names its before state outright instead of
	// tracking it, so this barrier would claim DEPTH_WRITE on a target that never left
	// PIXEL_SHADER_RESOURCE
	if (SceneDebugUI.AreShadowsEnabled())
	{
		SceneShadowAtlas.TransitionToShaderResource(CommandList);
	}

	// No depth here, the G-buffer already resolved which surface is visible
	const auto RtvHandle = SwapChain.GetRtvHandle(CurrentFrameIdx);
	CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);

	CommandList->SetGraphicsRootSignature(SceneLightPass.GetRootSignature());
	CommandList->SetPipelineState(SceneLightPass.GetPipelineState());

	// the G-buffer and the shadow map live in their own heap, so swap it in for the material one
	DXDescriptorHeap* LightHeaps[] = { LightInputsHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(LightHeaps), LightHeaps);

	CommandList->SetGraphicsRootDescriptorTable(SceneLightPass.GBufferTableIdx,	LightInputsHeap->GetGPUDescriptorHandleForHeapStart());
	CommandList->SetGraphicsRootConstantBufferView(SceneLightPass.CameraCBVIdx,	CameraCBV->GetGPUVirtualAddress() + (UINT64(CurrentFrameIdx) * CameraCbvStride));
	CommandList->SetGraphicsRootConstantBufferView(SceneLightPass.LightsCBVIdx,	LightsCBV->GetGPUVirtualAddress() + (UINT64(CurrentFrameIdx) * LightsCbvStride));

	SceneLightPass.RenderFullScreen(CommandList);
}

void D3D12Renderer::RenderTangentFramePass()
{
	const auto& Settings = SceneDebugUI.GetTangentFrameSettings();

	// Off, or no vector picked
	if (!Settings.bEnabled || Settings.GetVectorMask() == 0)
	{
		return;
	}

	// Objects that asked for their frame. Counted up front so that nothing flagged skips the
	// pass entirely, not even the barrier and the target switch, and so the marker can name it.
	SIZE_T FlaggedCount = 0;
	for (const auto& Object : Primitives)
	{
		FlaggedCount += Object->bShowTangentFrame ? 1 : 0;
	}

	if (FlaggedCount == 0)
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::TANGENT_FRAME, L"Tangent Frame | %zu of %zu objects", FlaggedCount, Primitives.size());
	{
		// The light pass left the depth readable as a texture. The overlay tests against it,
		// which wants it as a depth target again, and it never writes to it.
		DepthBuffer.TransitionToDepthWrite(CommandList);

		const auto RtvHandle = SwapChain.GetRtvHandle(CurrentFrameIdx);
		const auto DsvHandle = DepthBuffer.GetDsvHandle();
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, &DsvHandle);

		CommandList->SetGraphicsRootSignature(SceneTangentFramePass.GetRootSignature());
		CommandList->SetPipelineState(SceneTangentFramePass.GetPipelineStateFor(Settings.bDepthTest));
		// BeginFrame already set triangles, which is what the quads need, but a pass should
		// not lean on what an earlier one happened to leave behind
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// The camera's matrix is the same for every primitive, only the object differs. The
		// vector end points are offsets in world space, so they need the camera's own matrix
		// and not the per object WorldToClip. Transposed for HLSL like every other matrix.
		TangentFrameConstants ViewConstants = {};
		XMStoreFloat4x4(&ViewConstants.WorldToCameraClip, XMMatrixTranspose(Camera.GetViewMatrix() * Camera.GetProjMatrix()));
		ViewConstants.VectorLength	= Settings.VectorLength;
		ViewConstants.VertexFraction = Settings.GetVertexFraction();
		ViewConstants.LineThickness	= Settings.LineThickness;
		ViewConstants.ViewportSize	= { ViewPort.Width, ViewPort.Height };
		ViewConstants.VectorMask	= Settings.GetVectorMask();
		CommandList->SetGraphicsRoot32BitConstants(SceneTangentFramePass.ViewIdx, TangentFrameConstantCount, &ViewConstants, 0);

		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			if (!Primitives[PrimitiveIdx]->bShowTangentFrame)
			{
				continue;
			}

			CommandList->SetGraphicsRootConstantBufferView(SceneTangentFramePass.PrimitiveCBVIdx, GetPrimitiveCbvAddress(PrimitiveIdx));
			SceneTangentFramePass.RenderPrimitive(CommandList, *Primitives[PrimitiveIdx]);
		}

		// Back to what the UI expects: the back buffer, no depth. Its pipeline has no depth
		// format, and the debug layer rejects a draw whose target disagrees with the state.
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::RenderAxisGizmoPass()
{
	const auto& Settings = SceneDebugUI.GetAxisGizmoSettings();

	if (!Settings.bEnabled)
	{
		return;
	}

	// Objects that asked for their axes. Counted up front so that nothing flagged skips the
	// pass entirely, not even the barrier and the target switch, and so the marker can name it.
	SIZE_T FlaggedCount = 0;
	for (const auto& Object : Primitives)
	{
		FlaggedCount += Object->bShowOriginAxes ? 1 : 0;
	}

	if (FlaggedCount == 0)
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::AXIS_GIZMO, L"Origin Axes | %zu of %zu objects", FlaggedCount, Primitives.size());
	{
		// Depth as a depth target again, the light pass left it readable as a texture. Tested
		// against when the panel asks for it, never written to.
		DepthBuffer.TransitionToDepthWrite(CommandList);

		const auto RtvHandle = SwapChain.GetRtvHandle(CurrentFrameIdx);
		const auto DsvHandle = DepthBuffer.GetDsvHandle();
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, &DsvHandle);

		CommandList->SetGraphicsRootSignature(SceneAxisGizmoPass.GetRootSignature());
		CommandList->SetPipelineState(SceneAxisGizmoPass.GetPipelineStateFor(Settings.bDepthTest));
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// The camera's matrix is the same for every object, only the origin differs. An arrow
		// tip is an offset in world space from the origin, so it needs the camera's own matrix
		// and not the per object WorldToClip. Transposed for HLSL like every other matrix.
		AxisGizmoConstants ViewConstants = {};
		XMStoreFloat4x4(&ViewConstants.WorldToCameraClip, XMMatrixTranspose(Camera.GetViewMatrix() * Camera.GetProjMatrix()));
		ViewConstants.AxisLength	= Settings.AxisLength;
		ViewConstants.LineThickness	= Settings.LineThickness;
		ViewConstants.HeadSize		= Settings.HeadSize;
		ViewConstants.ViewportSize	= { ViewPort.Width, ViewPort.Height };
		CommandList->SetGraphicsRoot32BitConstants(SceneAxisGizmoPass.ViewIdx, AxisGizmoConstantCount, &ViewConstants, 0);

		for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
		{
			if (!Primitives[PrimitiveIdx]->bShowOriginAxes)
			{
				continue;
			}

			CommandList->SetGraphicsRootConstantBufferView(SceneAxisGizmoPass.PrimitiveCBVIdx, GetPrimitiveCbvAddress(PrimitiveIdx));
			SceneAxisGizmoPass.RenderPrimitive(CommandList, *Primitives[PrimitiveIdx]);
		}

		// Back to what the UI expects: the back buffer, no depth. Its pipeline has no depth
		// format, and the debug layer rejects a draw whose target disagrees with the state.
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::RenderSelectionPass()
{
	const auto& Settings = SceneDebugUI.GetSelectionSettings();
	const Primitive * const Selected = GetSelectedPrimitive();

	if (!Settings.bEnabled || Selected == nullptr || (!Settings.bShowTint && !Settings.bShowBox))
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();
	const SIZE_T SelectedIdx = static_cast<SIZE_T>(SceneDebugUI.GetSelectedPrimitiveIdx());

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::SELECTION, L"Selection | %s", Selected->Name.c_str());
	{
		// Depth as a depth target again, the light pass left it readable as a texture. The tint
		// tests against it, nothing here writes to it.
		DepthBuffer.TransitionToDepthWrite(CommandList);

		const auto RtvHandle = SwapChain.GetRtvHandle(CurrentFrameIdx);
		const auto DsvHandle = DepthBuffer.GetDsvHandle();
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, &DsvHandle);

		CommandList->SetGraphicsRootSignature(SceneSelectionPass.GetRootSignature());
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// One set of constants and one object buffer serve both pipelines, they share the root signature
		SelectionConstants ViewConstants = {};
		ViewConstants.BoxMin					= Selected->VertexFactory.LocalBoundsMin;
		ViewConstants.BoxMax					= Selected->VertexFactory.LocalBoundsMax;
		ViewConstants.SelectionLineThickness	= Settings.BoxThickness;
		ViewConstants.SelectionColor			= { Settings.Color.x, Settings.Color.y, Settings.Color.z, Settings.TintStrength };
		ViewConstants.SelectionViewport			= { ViewPort.Width, ViewPort.Height };
		CommandList->SetGraphicsRoot32BitConstants(SceneSelectionPass.ViewIdx, SelectionConstantCount, &ViewConstants, 0);
		CommandList->SetGraphicsRootConstantBufferView(SceneSelectionPass.PrimitiveCBVIdx, GetPrimitiveCbvAddress(SelectedIdx));

		// Tint first so the box, which ignores depth, is drawn over it
		if (Settings.bShowTint)
		{
			CommandList->SetPipelineState(SceneSelectionPass.GetPipelineState());
			SceneSelectionPass.RenderPrimitive(CommandList, *Selected);
		}

		// A mesh without vertices has no bounds, which the cache leaves inverted. Nothing to box then.
		const auto& BoundsMin = Selected->VertexFactory.LocalBoundsMin;
		const auto& BoundsMax = Selected->VertexFactory.LocalBoundsMax;
		if (Settings.bShowBox && BoundsMin.x <= BoundsMax.x && BoundsMin.y <= BoundsMax.y && BoundsMin.z <= BoundsMax.z)
		{
			CommandList->SetPipelineState(SceneSelectionPass.GetBoxPipelineState());
			SceneSelectionPass.RenderBox(CommandList);
		}

		// Back to what the next pass expects: the back buffer, no depth
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::RenderTransformGizmoPass()
{
	const auto& Settings = SceneDebugUI.GetTransformGizmoSettings();
	const Primitive * const Selected = GetSelectedPrimitive();

	if (!Settings.bEnabled || Selected == nullptr)
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::TRANSFORM_GIZMO, L"Transform Gizmo | %s", Selected->Name.c_str());
	{
		// The same frame the mouse is tested against, so what is drawn is what can be grabbed
		const GizmoFrame Frame = TransformGizmo::BuildFrame(BuildGizmoCamera(), *Selected, Settings);

		// No depth, the handles are drawn over everything and leave the depth buffer alone
		const auto RtvHandle = SwapChain.GetRtvHandle(CurrentFrameIdx);
		CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);

		CommandList->SetGraphicsRootSignature(SceneTransformGizmoPass.GetRootSignature());
		CommandList->SetPipelineState(SceneTransformGizmoPass.GetPipelineState());
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		const auto& CameraPosition = Camera.GetPosition();

		TransformGizmoConstants ViewConstants = {};
		XMStoreFloat4x4(&ViewConstants.WorldToCameraClip, XMMatrixTranspose(Camera.GetViewMatrix() * Camera.GetProjMatrix()));
		ViewConstants.GizmoOrigin			= Frame.Origin;
		ViewConstants.GizmoLength			= Frame.Length;
		ViewConstants.GizmoAxisX			= Frame.Axes[0];
		ViewConstants.GizmoAxisY			= Frame.Axes[1];
		ViewConstants.GizmoAxisZ			= Frame.Axes[2];
		ViewConstants.GizmoMode				= static_cast<UINT>(Settings.Mode);
		ViewConstants.GizmoHoveredAxis		= SceneTransformGizmo.GetHoveredAxis();
		ViewConstants.GizmoActiveAxis		= SceneTransformGizmo.GetActiveAxis();
		ViewConstants.GizmoCameraPosition	= { CameraPosition.x, CameraPosition.y, CameraPosition.z };
		ViewConstants.GizmoLineThickness	= Settings.LineThickness;
		ViewConstants.GizmoViewport			= { ViewPort.Width, ViewPort.Height };
		ViewConstants.GizmoHeadSize			= Settings.HeadSize;
		CommandList->SetGraphicsRoot32BitConstants(SceneTransformGizmoPass.ViewIdx, TransformGizmoConstantCount, &ViewConstants, 0);

		SceneTransformGizmoPass.RenderGizmo(CommandList, Settings.Mode);
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::RenderDebugUI()
{
	if (!UsesDebugUI())
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::UI, L"Debug UI");
	{
		// Straight over the shaded frame. LightPass already left the back buffer bound and
		// there is no depth, so the panel is simply drawn on top.
		SceneDebugUI.Draw(Camera, Catalog, Primitives, Lights, Materials, DefaultMaterial.get(), StepTimer.GetFramesPerSecond());
		SceneDebugUI.Render(CommandList);
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::EndFrame()
{
	auto * const CommandList = FrameContext.GetCommandList();

	// Indicate that the back buffer will now be used to present
	SwapChain.TransitionToPresent(CommandList, CurrentFrameIdx);

	FrameContext.Execute(Device, CurrentFrameIdx);
	SwapChain.Present(bVSyncEnabled ? 1 : 0);

	// No wait here on purpose. Execute() signalled this frame's fence, and BeginFrame()
	// blocks on it only once this frame index comes back around, which leaves the CPU
// free to record the next frames in the meantime.
	// The one exception is a frame that holds the copies for a dump, see FinishFrameDump
	FinishFrameDump();
}

void D3D12Renderer::CaptureFrameDump()
{
	if (!SceneDebugUI.ConsumeDumpRequest())
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::FRAME_DUMP, L"Frame Dump");
	{
		// Depth is turned back into a distance with the entries of the projection that make z
		XMFLOAT4X4 Projection;
		XMStoreFloat4x4(&Projection, Camera.GetProjMatrix());

		// The back buffer is still the render target LightPass drew into
		SceneFrameDump.Capture(Device, CommandList, SwapChain, CurrentFrameIdx, D3D12_RESOURCE_STATE_RENDER_TARGET,
								SceneGBuffer, DepthBuffer, Projection._33, Projection._43);
		bFrameDumpRecorded = true;
	}
	DXI_PIX_END(CommandList);
}

void D3D12Renderer::FinishFrameDump()
{
	if (!bFrameDumpRecorded)
	{
		return;
	}
	bFrameDumpRecorded = false;

	// The copies sit on the list that was just submitted. Reading before the GPU has run them
	// would hand back empty memory, so this is the one frame where the CPU stands and waits.
	FrameContext.WaitForFrame(Device, CurrentFrameIdx);

	const auto Result = SceneFrameDump.Write((DXIPATHS::GetExecutableDir / L"Dumps").Str());

	SceneDebugUI.SetDumpStatus(Result.Message, Result.bSuccess);
	OutputDebugStringW((L"[FrameDump] " + Result.Message + L"\n").c_str());
}

void D3D12Renderer::UploadCameraConstants()
{
	DXASSERT(CameraCbvAddress, "Camera constant buffer must be mapped before uploading into it");

	CameraConstantBuffer CameraCbv	= {};
	CameraCbv.ViewPosition			= Camera.GetPosition();

	const auto& Pos					= CameraCbv.ViewPosition;
	CameraCbv.PreViewTranslation	= { -Pos.x, -Pos.y, -Pos.z, 0.f };

	const auto View			= Camera.GetViewMatrix();
	const auto Proj			= Camera.GetProjMatrix();
	const auto ViewProj		= View * Proj;

	// Pre translating keeps float precision where it is needed. A world position far from the
	// origin spends its mantissa on magnitude, and the view transform then subtracts that large
	// part away again, leaving the precision it cost behind as vertex jitter. Camera relative
	// positions stay small, so the precision lands near the camera where it shows.
	//
	// World -> TranslatedWorld adds PreViewTranslation, which leaves TranslatedWorld -> View as
	// rotation only. Lifted from the view matrix as FViewMatrices does, rather than multiplied
	// back out of it: the two translations cancel on paper, but in float they leave a residue
	// that grows with distance, 16 mm at 1000 km, which is the error the translation exists to
	// avoid in the first place.
	XMMATRIX ViewTranslated			= View;
	ViewTranslated.r[3]				= g_XMIdentityR3;
	const auto ViewProjTranslated	= ViewTranslated * Proj;

	// Rotation only, so the inverse is the transpose, and composing the two beats inverting
	// their product. FViewMatrices builds InvTranslatedViewProjectionMatrix the same way
	const auto InvViewProjTranslated = XMMatrixInverse(nullptr, Proj) * XMMatrixTranspose(ViewTranslated);

	// Transposed on the way out, like every other matrix the GPU reads
	XMStoreFloat4x4(&CameraCbv.ViewProj,				XMMatrixTranspose(ViewProj));
	XMStoreFloat4x4(&CameraCbv.InvViewProj,				XMMatrixTranspose(XMMatrixInverse(nullptr, ViewProj)));
	XMStoreFloat4x4(&CameraCbv.ViewProjTranslated,		XMMatrixTranspose(ViewProjTranslated));
	XMStoreFloat4x4(&CameraCbv.InvViewProjTranslated,	XMMatrixTranspose(InvViewProjTranslated));

	memcpy(CameraCbvAddress + (SIZE_T(CurrentFrameIdx) * CameraCbvStride), &CameraCbv, sizeof(CameraCbv));
}

void D3D12Renderer::AddLight(std::unique_ptr<LightBase> Light)
{
	DXASSERT(Light, "A null light can not be placed in the scene");

	// Only the first directional light becomes the sun. A second one appends like anything
	// else rather than displacing the slot the shaders already read
	if (Light->GetLightType() == LightType::Directional && !HasSunLight())
	{
		Lights.insert(Lights.begin(), std::move(Light));
	}
	else
	{
		Lights.emplace_back(std::move(Light));
	}

	DXASSERT(Lights.size() <= MAX_LIGHTS, "More lights placed in the scene than the constant buffer can hold");
}

bool D3D12Renderer::HasSunLight() const noexcept
{
	return !Lights.empty() && Lights.front()->GetLightType() == LightType::Directional;
}

void D3D12Renderer::UploadLightConstants()
{
	DXASSERT(LightsCbvAddress, "Lights constant buffer must be mapped before uploading into it");

	LightsConstantBuffer LightsCbv	= {};
	LightsCbv.NumLights				= static_cast<UINT>(Lights.size() < MAX_LIGHTS ? Lights.size() : MAX_LIGHTS);

	for (UINT LightIdx = 0; LightIdx < LightsCbv.NumLights; ++LightIdx)
	{
		// Not named Light, that is now the shared struct type this loop writes into
		const auto& SceneLight = *Lights[LightIdx];

		// Get light constants i.e., per light pos, etc.
		LightsCbv.Lights[LightIdx] = SceneLight.GetLightConstants();

		// One slice per light, so the index is the slot. NO_SHADOW_CASTER already means
		// fully lit to GetShadowFactor, so shadows off needs no switch in the shaders
		LightsCbv.Lights[LightIdx].ShadowSlot = SceneDebugUI.AreShadowsEnabled()
													? static_cast<INT>(LightIdx)
													: NO_SHADOW_CASTER;

		// Only the single view kinds need a matrix here. A point light is looked up by
		// direction, so its six face matrices never leave the shadow pass.
		if (SceneLight.GetLightType() != LightType::Point)
		{
			XMStoreFloat4x4(&LightsCbv.ShadowViewProj[LightIdx], XMMatrixTranspose(GetLightShadowViewProj(SceneLight)));
		}
		else
		{
			XMStoreFloat4x4(&LightsCbv.ShadowViewProj[LightIdx], XMMatrixIdentity());
		}
	}

	memcpy(LightsCbvAddress + (SIZE_T(CurrentFrameIdx) * LightsCbvStride), &LightsCbv, sizeof(LightsCbv));
}

void D3D12Renderer::UploadPrimitiveConstants()
{
	DXASSERT(PrimitiveCbvAddress, "Primitive constant buffer must be mapped before uploading into it");

	for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
	{

		// View and Proj mat per frame (camera can move every frame, so we need to update these every frame)
		const auto ViewMat = Camera.GetViewMatrix();
		const auto ProjMat = Camera.GetProjMatrix();

		// World Mat for Primitive
		const auto PrimitiveWorldMatrix = Primitives[PrimitiveIdx]->GetWorldMatrix();

		// WorldToClip (Remember to transpose for HLSL/GPU)
		const XMMATRIX LocalToWorld	= XMLoadFloat4x4(&PrimitiveWorldMatrix);
		const XMMATRIX WorldToClip	= LocalToWorld * ViewMat * ProjMat;

		PrimitiveConstantBuffer PrimitiveCbv = {};

		// Store Primitive matrices in transposed form for HLSL/GPU
		XMStoreFloat4x4(&PrimitiveCbv.LocalToWorld, XMMatrixTranspose(LocalToWorld));
		XMStoreFloat4x4(&PrimitiveCbv.WorldToClip, XMMatrixTranspose(WorldToClip));

		// Inverse of the full world matrix, and not transposed on the way in. Every other
		// matrix is transposed so HLSL's column major layout sees it as stored. Skipping
		// that transpose here leaves the shader with the inverse transpose, which keeps a
		// normal perpendicular when the scale is uneven. The shader passes the normal with
		// w of 0, so the translation in this matrix drops out.
		XMStoreFloat4x4(&PrimitiveCbv.NormalToWorld, XMMatrixInverse(nullptr, LocalToWorld));


		// PrevWorldToClip for VelocityPass, transposed for HLSL/GPU
		const auto PrevPrimitiveWorldMatrix = Primitives[PrimitiveIdx]->GetPrevWorldMatrix();
		const XMMATRIX PrevWorldToClip = XMLoadFloat4x4(&PrevPrimitiveWorldMatrix) * Camera.GetPrevViewProjMatrix();
		XMStoreFloat4x4(&PrimitiveCbv.PrevWorldToClip, XMMatrixTranspose(PrevWorldToClip));

		// No light matrix here any more. ShadowPass renders the same object from several
		// lights per frame, so it takes the light half as root constants instead.

		// The stride between frame regions is the capacity, not the current count. Using the
		// count would move every region the moment the panel adds a primitive, leaving the
		// frames still in flight reading from the wrong offsets.
		const SIZE_T FrameOffset = SIZE_T(CurrentFrameIdx) * MAX_PRIMITIVES * PrimitiveCbvStride;
		memcpy(PrimitiveCbvAddress + FrameOffset + (SIZE_T(PrimitiveIdx) * PrimitiveCbvStride), &PrimitiveCbv, PrimitiveConstantBufferSize);
	}
}

void D3D12Renderer::VelocityPass()
{
	for (auto& Primitive : Primitives)
	{
		Primitive->AdvanceFrame();
	}

	Camera.AdvanceFrame();
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Renderer::GetPrimitiveCbvAddress(const SIZE_T PrimitiveIdx) const
{
	DXASSERT(PrimitiveCBV, "Primitive constant buffer must be created before binding it");
	DXASSERT(PrimitiveIdx < Primitives.size(), "Primitive index is out of range");

	const UINT64 FrameOffset = UINT64(CurrentFrameIdx) * MAX_PRIMITIVES * PrimitiveCbvStride;
	return PrimitiveCBV->GetGPUVirtualAddress() + FrameOffset + (UINT64(PrimitiveIdx) * PrimitiveCbvStride);
}

void D3D12Renderer::Destroy()
{
	// Nothing may be released while the GPU is still reading it. Frames are no longer
	// waited on after Present, so on the way out there can be several still in flight,
	// and letting the swap chain or any resource go here would be reading freed memory.
	FrameContext.WaitForGpuIdle(Device);

	// Stops Update() and Render() from touching anything after this point. A WM_PAINT can
	// still be sitting in the queue when WM_DESTROY arrives, and rendering from it would
	// use constant buffers that have just been unmapped.
	bDestroyed = true;

	// Win32Proc calls Destroy() on WM_DESTROY and again once the message loop ends,
	// so every step here has to tolerate running twice
	const auto UnmapBuffer = [](ComPtr<DXResource>& Buffer, UINT8** Address)
	{
		if (Buffer && *Address)
		{
			Buffer->Unmap(0, nullptr);
			*Address = nullptr;
		}
	};

	UnmapBuffer(PrimitiveCBV,	&PrimitiveCbvAddress);
	UnmapBuffer(CameraCBV,		&CameraCbvAddress);
	UnmapBuffer(LightsCBV,		&LightsCbvAddress);

	Device.ReportLiveObjects();
}

void D3D12Renderer::OnKeyDown(UINT8 Key)
{
	// Tool hotkeys. Neither is a camera key, W A S D and the arrows are those.
	switch (Key)
	{
	case '1':
		SceneDebugUI.GetTransformGizmoSettings().Mode = TransformMode::Move;
		return;

	case '2':
		SceneDebugUI.GetTransformGizmoSettings().Mode = TransformMode::Rotate;
		return;

	case VK_ESCAPE:
		// Lets go of a held handle as well, the object stays where it was dragged to
		SceneTransformGizmo.EndDrag();
		SceneTransformGizmo.ClearHover();
		SceneDebugUI.SetSelectedPrimitiveIdx(-1);
		return;

	default:;
	}

	Camera.OnKeyDown(Key);
}

void D3D12Renderer::OnKeyUp(UINT8 Key)
{
	Camera.OnKeyUp(Key);
}

void D3D12Renderer::OnMouseMove(int DeltaX, int DeltaY)
{
	Camera.OnMouseMove(DeltaX, DeltaY);
}

Primitive* D3D12Renderer::GetSelectedPrimitive() const
{
	const int SelectedIdx = SceneDebugUI.GetSelectedPrimitiveIdx();

	// The panel clamps its own index, but a primitive can go between that and here
	if (SelectedIdx < 0 || SelectedIdx >= static_cast<int>(Primitives.size()))
	{
		return nullptr;
	}

	return Primitives[static_cast<size_t>(SelectedIdx)].get();
}

GizmoCamera D3D12Renderer::BuildGizmoCamera() const
{
	const auto& CameraPosition = Camera.GetPosition();

	return MakeGizmoCamera(Camera.GetViewMatrix(),
							Camera.GetProjMatrix(),
							XMFLOAT3(CameraPosition.x, CameraPosition.y, CameraPosition.z),
							XMFLOAT2(ViewPort.Width, ViewPort.Height));
}

XMFLOAT2 D3D12Renderer::ClientToViewport(const int X, const int Y) const
{
	// The window can be a different size than what is rendered, the viewport is always the render
	// size, so the cursor is scaled to it
	RECT Client = {};
	GetClientRect(WindowHandle, &Client);

	const float ClientWidth		= static_cast<float>(Client.right - Client.left);
	const float ClientHeight	= static_cast<float>(Client.bottom - Client.top);

	if (ClientWidth < 1.f || ClientHeight < 1.f)
	{
		return XMFLOAT2(static_cast<float>(X), static_cast<float>(Y));
	}

	return XMFLOAT2(static_cast<float>(X) * ViewPort.Width / ClientWidth,
					static_cast<float>(Y) * ViewPort.Height / ClientHeight);
}

void D3D12Renderer::OnLeftButtonDown(int X, int Y)
{
	CursorPixels = ClientToViewport(X, Y);

	// The handles of what is already selected win over whatever is behind them, they are drawn on top.
	// A press that is not on one falls through to picking.
	const auto& Settings = SceneDebugUI.GetTransformGizmoSettings();
	if (Primitive * const Selected = GetSelectedPrimitive())
	{
		if (Settings.bEnabled && SceneTransformGizmo.BeginDrag(BuildGizmoCamera(), *Selected, Settings, CursorPixels))
		{
			return;
		}
	}

	// Nothing under the cursor deselects, which -1 is
	const PickRay Ray = BuildPickRay(CursorPixels,
									XMFLOAT2(ViewPort.Width, ViewPort.Height),
									Camera.GetViewMatrix(),
									Camera.GetProjMatrix());
	SceneDebugUI.SetSelectedPrimitiveIdx(PickPrimitive(Ray, Primitives));
	SceneTransformGizmo.ClearHover();
}

void D3D12Renderer::OnLeftButtonUp(int X, int Y)
{
	SceneTransformGizmo.EndDrag();

	// The handle under the cursor lights up again right away, without waiting for the next move
	OnCursorMove(X, Y);
}

void D3D12Renderer::OnCursorMove(int X, int Y)
{
	CursorPixels = ClientToViewport(X, Y);

	const auto& Settings = SceneDebugUI.GetTransformGizmoSettings();
	Primitive * const Selected = GetSelectedPrimitive();

	// A held handle follows the cursor wherever it goes, over a panel too
	if (SceneTransformGizmo.IsDragging())
	{
		if (Selected != nullptr)
		{
			// Ctrl rounds the move or the turn to a step
			const bool bSnap = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
			SceneTransformGizmo.Drag(BuildGizmoCamera(), *Selected, Settings, CursorPixels, bSnap);
		}
		else
		{
			SceneTransformGizmo.EndDrag();
		}
		return;
	}

	// Pointing at a panel is not pointing at the view behind it
	if (Selected == nullptr || !Settings.bEnabled || DebugUI::WantsMouse())
	{
		SceneTransformGizmo.ClearHover();
		return;
	}

	SceneTransformGizmo.UpdateHover(BuildGizmoCamera(), *Selected, Settings, CursorPixels);
}
