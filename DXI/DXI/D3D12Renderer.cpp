//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "D3D12Renderer.h"

// Own Includes
#include "Win32Proc.h"
#include "Paths.h"
#include "Objects/Model.h"

#include <string>
#include <pix3.h>

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
	// Core | Device first, everything else is built on top of it
	Device.Init();
	SwapChain.Init(Device, Win32Proc::GetHwnd(), WindowWidth, WindowHeight, BACK_BUFFER_COUNT);
	DepthBuffer.Init(Device, WindowWidth, WindowHeight);
	FrameContext.Init(Device, BACK_BUFFER_COUNT);

	CurrentFrameIdx = SwapChain.GetCurrentBackBufferIdx();

	// Scene records uploads into the command list, so it has to come before the flush below
	InitializeScene();
	InitializeConstantBuffers();
	InitializePasses();

	// Debug panel. It uploads a font atlas of its own, so it goes in before the flush.
	SceneDebugUI.Init(Device.Get(), Win32Proc::GetHwnd(), D3D12SwapChain::GetFormat(), BACK_BUFFER_COUNT);

	// Push the vertex, index and texture uploads to the GPU and wait them out
	FrameContext.Flush(Device, CurrentFrameIdx);
}

void D3D12Renderer::InitializeScene()
{
	auto * const CommandList = FrameContext.GetCommandList();

	InitializeMaterials();

	/***************************
	 ********** Sky ************
	 ***************************/
	// CC0 panorama from Poly Haven. It is equirectangular, so the six cube faces get
	// projected out of it here rather than being shipped as separate images.
	const auto PanoramaPath = (DXIPATHS::GetTextureDir / L"sky" / L"cloud_layers.jpg").Str();

	// The sky as it looks, mipped so a rough surface reflects a blurry version of it
	SkyRadianceMap.LoadRadiance(Device.Get(),
								CommandList,
								PanoramaPath,
								SKY_FACE_RESOLUTION,
								L"Sky Radiance CubeMap");

	// The same sky gathered over the hemisphere, which is what a diffuse surface receives
	SkyIrradianceMap.LoadIrradiance(Device.Get(),
									CommandList,
									PanoramaPath,
									SKY_IRRADIANCE_RESOLUTION,
									L"Sky Irradiance CubeMap");

	/***************************
	 ******* Primitives ********
	 ***************************/
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube1")));
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube2")));
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube3")));
	Primitives.emplace_back(std::make_unique<Plane>(TEXT("Ground")));
	Primitives.emplace_back(std::make_unique<Cube>(TEXT("Cube4")));

	// Spot, by Keenan Crane, dedicated to the public domain under CC0 1.0. The mesh
	// carries uvs but no normals, which suits a renderer that computes its own per face.
	Primitives.emplace_back(std::make_unique<Model>(TEXT("Spot"),
								(DXIPATHS::GetModelDir / L"spot" / L"spot_triangulated.obj").Str()));

	for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
	{
		// Init Vertex/Index Buffers
		Primitives[PrimitiveIdx]->Init(Device.Get(), CommandList);

		// Materials are handed out by index, so no two objects look the same. Any
		// primitive past the end of the list keeps the default material.
		if (PrimitiveIdx < Materials.size())
		{
			Primitives[PrimitiveIdx]->SetMaterial(Materials[PrimitiveIdx].get());
		}
	}

	// Cubes in a row, the last primitive is the ground laid out underneath them
	Primitives[0]->Transform({ -3.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
	Primitives[1]->Transform({ -1.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
	Primitives[2]->Transform({  1.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
	Primitives[3]->Transform({  0.0f, 0.0f, 3.0f }, { 90.f, 0.0f, 0.0f }, { 20.f, 20.f, 1.f });
	Primitives[4]->Transform({  10.0f, 0.0f, 10.0f }, { 90.f, 0.0f, 0.0f }, { 1.f, 1.f, 1.f });

	// Spot sits in front of the cubes. The mesh is modelled about a unit tall and lying
	// along +Z, so it is turned to face the camera and lifted clear of the ground.
	Primitives[5]->Transform({  3.0f, 1.0f, 1.0f }, { 0.0f, -90.0f, 0.0f }, { 1.5f, 1.5f, 1.5f });

	/***************************
	 ********** Lights *********
	 ***************************/
	{
		// Sun above the scene, shining down from the left and behind
		auto Sun = std::make_unique<DirectionalLight>(TEXT("DirectionalLight1"));
		Sun->SetDirection({ -0.5f, -1.0f, 0.5f });
		Sun->SetColor({ 1.0f, 0.95f, 0.8f });
		Sun->SetIntensity(0.6f);

		// The shadow box has to contain everything that should cast, the cubes sit around
		// the origin and the ground reaches a few units out
		Sun->SetShadowBounds({ 0.0f, 0.0f, 1.0f }, 20.f, 20.f);
		Lights.emplace_back(std::move(Sun));

		// Bulb sitting in front of the cubes
		auto Bulb = std::make_unique<PointLight>(TEXT("PointLight1"));
		Bulb->SetPosition({ 2.0f, 1.0f, -1.0f });
		Bulb->SetColor({ 1.0f, 1.0f, 0.0f });

		// Intensity is now divided by the distance squared, so it is no longer roughly
		// the brightness at the surface. These are starting points, tune them in the panel.
		Bulb->SetIntensity(6.0f);
		Bulb->SetRange(8.0f);
		Bulb->SetSourceRadius(0.2f);
		Lights.emplace_back(std::move(Bulb));

		// Softbox hanging over the cubes, facing straight down
		auto Softbox = std::make_unique<AreaLight>(TEXT("AreaLight1"));
		Softbox->SetPosition({ -2.0f, 3.0f, 0.0f });
		Softbox->SetDirection({ 0.0f, -1.0f, 0.0f });
		Softbox->SetSize({ 2.0f, 2.0f });
		Softbox->SetColor({ 0.4f, 0.6f, 1.0f });

		// Same inverse square scale, and it sits further from the cubes than the bulb
		Softbox->SetIntensity(12.0f);
		Softbox->SetRange(12.0f);
		Lights.emplace_back(std::move(Softbox));

		DXASSERT(Lights.size() <= MAX_LIGHTS, "More lights placed in the scene than the constant buffer can hold");
	}
}

void D3D12Renderer::InitializeMaterials()
{
	// Every material takes its descriptors out of this single heap
	SharedMaterialHeap.Init(Device.Get(), MAX_MATERIALS);

	// Default first, so any mesh created without a material of its own still draws.
	// Kept out of Materials, which stays one entry per primitive.
	DefaultMaterial = std::make_unique<Material>(TEXT("Default Material"));
	DefaultMaterial->Init(Device.Get(), SharedMaterialHeap);
	DefaultMaterial->SetDefaults(Device.Get(), FrameContext.GetCommandList());

	// Primitive::GetMaterial() falls back to this one
	Primitive::SetDefaultMaterial(DefaultMaterial.get());

	// All CC0 from Poly Haven, in the order the primitives are created below, since
	// materials are handed out by index.
	LoadMaterial(L"american_walnut_veneer");	// Cube1
	LoadMaterial(L"brick_4");					// Cube2
	LoadMaterial(L"brushed_concrete");			// Cube3

	// The ground is a single quad with no thickness, so it has to survive being looked
	// at from underneath
	LoadMaterial(L"blue_floor_tiles_01").SetTwoSided(true);		// Ground

	LoadMaterial(L"metal_plate");				// Cube4

	// Spot ships one colour map and nothing else, so the remaining slots fall back to the
	// neutral values SetDefaults fills in
	CreateMaterialWithDiffuse(L"spot", (DXIPATHS::GetModelDir / L"spot" / L"spot_texture.png").Str());	// Spot

	DXASSERT(Materials.size() <= MAX_MATERIALS, "More materials loaded than the shared heap can hold");

	// What else is sitting on disk, so the panel can offer it
	Catalog.Scan();

	// Point the catalog at the materials already loaded above, otherwise picking one of
	// them in the panel would load a second copy and burn another heap slot
	for (auto& Asset : Catalog.GetMaterials())
	{
		for (SIZE_T MaterialIdx = 0; MaterialIdx < Materials.size(); ++MaterialIdx)
		{
			if (Materials[MaterialIdx]->GetName() == Asset.Name)
			{
				Asset.LoadedIdx = static_cast<int>(MaterialIdx);
				break;
			}
		}
	}
}

Material* D3D12Renderer::EnsureMaterialLoaded(const int CatalogIdx)
{
	auto& CatalogMaterials = Catalog.GetMaterials();

	if (CatalogIdx < 0 || CatalogIdx >= static_cast<int>(CatalogMaterials.size()))
	{
		return nullptr;		// -1 is the panel saying "keep the default"
	}

	auto& Asset = CatalogMaterials[static_cast<size_t>(CatalogIdx)];

	if (Asset.LoadedIdx >= 0)
	{
		return Materials[static_cast<size_t>(Asset.LoadedIdx)].get();
	}

	// One descriptor range per material, and the shared heap has a fixed number of them
	if (Materials.size() >= MAX_MATERIALS)
	{
		DXLOG("Shared material heap is full, cannot load another material\n")
		return nullptr;
	}

	// A folder of Poly Haven maps, or a lone colour map shipped beside a mesh
	Material& Loaded = Asset.DiffuseOnlyPath.empty()
							? LoadMaterial(Asset.Name.c_str())
							: CreateMaterialWithDiffuse(Asset.Name.c_str(), Asset.DiffuseOnlyPath);

	Asset.LoadedIdx = static_cast<int>(Materials.size()) - 1;
	return &Loaded;
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
		const auto& EyePosition	= Camera.GetPosition();
		const auto& Forward		= Camera.GetDirection();
		Spawned->SetTransform({ EyePosition.x + (Forward.x * 5.f),
								EyePosition.y + (Forward.y * 5.f),
								EyePosition.z + (Forward.z * 5.f) },
								DX_IDENTITY_ROTATE3,
								DX_IDENTITY_SCALE3);

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
		const auto& EyePosition	= Camera.GetPosition();

		switch (Request.Type)
		{
		case LightType::Directional:
		{
			auto Spawned = std::make_unique<DirectionalLight>(L"Directional " + Ordinal);
			Spawned->SetDirection({ -0.5f, -1.0f, 0.5f });
			Spawned->SetIntensity(0.6f);
			Lights.emplace_back(std::move(Spawned));
			break;
		}

		case LightType::Area:
		{
			auto Spawned = std::make_unique<AreaLight>(L"Area " + Ordinal);
			Spawned->SetPosition({ EyePosition.x, EyePosition.y, EyePosition.z });
			Spawned->SetDirection({ 0.0f, -1.0f, 0.0f });
			Spawned->SetIntensity(12.0f);
			Lights.emplace_back(std::move(Spawned));
			break;
		}

		default:
		{
			auto Spawned = std::make_unique<PointLight>(L"Point " + Ordinal);
			Spawned->SetPosition({ EyePosition.x, EyePosition.y, EyePosition.z });
			Spawned->SetIntensity(6.0f);
			Spawned->SetSourceRadius(0.2f);
			Lights.emplace_back(std::move(Spawned));
			break;
		}
		}
	}

	PrimitiveRequests.clear();
	LightRequests.clear();
}

Material& D3D12Renderer::CreateMaterialWithDiffuse(const wchar_t* Name, const std::wstring& DiffusePath)
{
	DXASSERT(Name, "Material needs a name");

	auto NewMaterial = std::make_unique<Material>(Name);
	NewMaterial->Init(Device.Get(), SharedMaterialHeap);
	NewMaterial->LoadTexture(Device.Get(), FrameContext.GetCommandList(), MaterialTextureSlot::Diffuse, DiffusePath);

	// Everything else neutral, so the surface shades as a plain dielectric
	NewMaterial->SetDefaults(Device.Get(), FrameContext.GetCommandList());

	Materials.emplace_back(std::move(NewMaterial));
	return *Materials.back();
}

Material& D3D12Renderer::LoadMaterial(const wchar_t* AssetName)
{
	DXASSERT(AssetName, "Material needs an asset name to load from");

	auto NewMaterial = std::make_unique<Material>(AssetName);
	NewMaterial->Init(Device.Get(), SharedMaterialHeap);

	// Poly Haven ships one file per map, all of them named after the asset.
	// nor_dx is the DirectX normal variant, so the green channel needs no flip.
	// Not every asset has every map, the missing ones keep their null descriptor.
	const std::wstring Name		= AssetName;
	const auto AssetDir			= DXIPATHS::GetTextureDir / Name;

	const struct
	{
		MaterialTextureSlot Slot;
		const wchar_t*		Suffix;
	}
	MapFiles[] =
	{
		{ MaterialTextureSlot::Diffuse,			L"_diff_1k.png"		},
		{ MaterialTextureSlot::Normal,			L"_nor_dx_1k.png"	},
		{ MaterialTextureSlot::Specular,		L"_spec_1k.png"		},
		{ MaterialTextureSlot::Metalness,		L"_metal_1k.png"	},
		{ MaterialTextureSlot::Roughness,		L"_rough_1k.png"	},
		{ MaterialTextureSlot::AmbientOcclusion,L"_ao_1k.png"		},
	};

	for (const auto& MapFile : MapFiles)
	{
		const auto MapPath = AssetDir / (Name + MapFile.Suffix);
		if (GetFileAttributesW(MapPath.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			continue;
		}

		NewMaterial->LoadTexture(Device.Get(), FrameContext.GetCommandList(), MapFile.Slot, MapPath.Str());
	}

	DXASSERT(NewMaterial->HasTexture(MaterialTextureSlot::Diffuse), "Material has no diffuse map, check the asset folder next to the executable");

	// Whatever the folder did not provide gets a neutral value, so no slot is left on the
	// null descriptor it started with
	NewMaterial->SetDefaults(Device.Get(), FrameContext.GetCommandList());

	Materials.emplace_back(std::move(NewMaterial));
	return *Materials.back();
}

void D3D12Renderer::InitializeConstantBuffers()
{
	DXASSERT(!Primitives.empty(), "Scene must be built before its constant buffers are sized");

	// Every buffer holds one region per frame in flight, so the GPU never reads what the CPU is writing
	const auto CreateMappedBuffer = [this](const UINT64 SizeInBytes, ComPtr<DXResource>& OutBuffer, UINT8** OutAddress, LPCWSTR Name)
	{
		const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes);
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
	const UINT64 PrimitiveRegionSize = static_cast<UINT64>(MAX_PRIMITIVES) * PrimitiveConstantBufferSize;

	CreateMappedBuffer(PrimitiveRegionSize * BACK_BUFFER_COUNT,					PrimitiveCBV,	&PrimitiveCbvAddress,	L"Primitive Constant Buffer");
	CreateMappedBuffer(UINT64(CAMERA_CBV_STRIDE) * BACK_BUFFER_COUNT,			CameraCBV,		&CameraCbvAddress,		L"Camera Constant Buffer");
	CreateMappedBuffer(UINT64(LightsConstantBufferSize) * BACK_BUFFER_COUNT,	LightsCBV,		&LightsCbvAddress,		L"Lights Constant Buffer");

	// Fill the first frame so nothing reads uninitialized memory before Update() runs
	UploadCameraConstants();
	UploadLightConstants();
	for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
	{
		UploadPrimitiveConstants(*Primitives[PrimitiveIdx], static_cast<UINT>(PrimitiveIdx));
	}
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

	// Only one CBV/SRV/UAV heap can be bound at a time, so everything LightPass samples
	// goes into a single heap: the G-buffer targets, the shadow map, then both sky cubes
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc	= {};
	HeapDesc.NumDescriptors				= LightPass::GetSrvCount();
	HeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(Device.Get()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&LightInputsHeap)));
	NAME_D3D12_OBJECT(LightInputsHeap, L"Light Pass Inputs Heap");

	const auto IncrementSize = Device.Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHandle(LightInputsHeap->GetCPUDescriptorHandleForHeapStart());

	SceneGBuffer.CreateShaderResourceViews(Device.Get(), SrvHandle, IncrementSize);

	SrvHandle.Offset(GBufferTargetCount, IncrementSize);
	SceneShadowAtlas.CreateShaderResourceViews(Device.Get(), SrvHandle, IncrementSize);

	SrvHandle.Offset(ShadowAtlas::GetSrvCount(), IncrementSize);
	SkyRadianceMap.CreateShaderResourceView(Device.Get(), SrvHandle);

	SrvHandle.Offset(1, IncrementSize);
	SkyIrradianceMap.CreateShaderResourceView(Device.Get(), SrvHandle);
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

	default:
		return XMMatrixIdentity();
	}
}

void D3D12Renderer::Update(float DeltaTime)
{
	if (bDestroyed)
	{
		return;
	}

	// Win32Proc hands us a zero delta, so the timer is the real source of elapsed time
	StepTimer.Tick(NULL);
	const auto ElapsedSeconds = static_cast<float>(StepTimer.GetElapsedSeconds());

	// Update window text with FPS value
	if (CurrentFrameIdx % 509)
	{
		wchar_t Fps[64];
		swprintf_s(Fps, L"%ufps", StepTimer.GetFramesPerSecond());

		// Saying which it is makes a frame rate pinned to the refresh rate obvious
		const std::wstring VSyncText	= bVSyncEnabled ? L" | VSync On" : L" | VSync Off";
		const std::wstring WindowText	= WindowName + L" | Deferred" + VSyncText + L" | FPS: " + Fps;
		SetWindowText(Win32Proc::GetHwnd(), WindowText.c_str());
	}

	// Camera matrices
	Camera.Update(ElapsedSeconds);

	// NOTE | The constant uploads deliberately do not happen here. Update() runs before
	// BeginFrame() picks the frame index, so writing them now would target the region the
	// GPU may still be reading. Render() uploads them once the index is known.
}

void D3D12Renderer::Render()
{
	if (bDestroyed)
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
	for (SIZE_T PrimitiveIdx = 0; PrimitiveIdx < Primitives.size(); ++PrimitiveIdx)
	{
		UploadPrimitiveConstants(*Primitives[PrimitiveIdx], static_cast<UINT>(PrimitiveIdx));
	}

	RenderShadowPass();
	RenderBasePass();
	RenderLightPass();
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

	DepthBuffer.Clear(CommandList);

	// Opens the UI frame, the widgets are filled in once the passes are done
	SceneDebugUI.BeginFrame();
}

void D3D12Renderer::RenderShadowPass()
{
	if (Lights.empty())
	{
		return;
	}

	auto * const CommandList = FrameContext.GetCommandList();

	// The scene from every light in turn. A directional or area light is one depth view,
	// a point light is six faces of a cube, so this is up to MAX_LIGHTS * 6 draws of the
	// whole scene per frame.
	PIXBeginEvent(CommandList, 0, L"Shadow Pass");
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

		const auto CasterCount = Lights.size() < MAX_LIGHTS ? Lights.size() : MAX_LIGHTS;

		for (UINT LightIdx = 0; LightIdx < static_cast<UINT>(CasterCount); ++LightIdx)
		{
			const auto& Light = *Lights[LightIdx];

			PIXBeginEvent(CommandList, 0, Light.GetName().c_str());
			if (Light.GetLightType() == LightType::Point)
			{
				// Distance into a colour target, one face at a time
				CommandList->SetPipelineState(SceneShadowPass.GetDistancePipelineState());

				for (UINT FaceIdx = 0; FaceIdx < ShadowAtlas::CUBE_FACE_COUNT; ++FaceIdx)
				{
					SceneShadowAtlas.BindCubeFace(CommandList, LightIdx, FaceIdx);
					RenderView(Light, FaceIdx);
				}
			}
			else
			{
				// Depth only, a single view is enough
				CommandList->SetPipelineState(SceneShadowPass.GetPipelineState());

				SceneShadowAtlas.BindDepthSlice(CommandList, LightIdx);
				RenderView(Light, 0);
			}
			PIXEndEvent(CommandList);
		}

		// Back to the screen sized viewport for the rest of the frame
		CommandList->RSSetViewports(1, &ViewPort);
		CommandList->RSSetScissorRects(1, &ScissorRect);
	}
	PIXEndEvent(CommandList);
}

void D3D12Renderer::RenderBasePass()
{
	auto * const CommandList = FrameContext.GetCommandList();

	// Surface only, every primitive lands in the G-buffer with its material applied and no lighting
	PIXBeginEvent(CommandList, 0, L"Base Pass");
	{
		SceneGBuffer.TransitionToRenderTarget(CommandList);
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
	PIXEndEvent(CommandList);
}

void D3D12Renderer::RenderLightPass()
{
	auto * const CommandList = FrameContext.GetCommandList();

	// Shades what BasePass wrote, straight into the back buffer
	SceneGBuffer.TransitionToShaderResource(CommandList);
	SceneShadowAtlas.TransitionToShaderResource(CommandList);

	// No depth here, the G-buffer already resolved which surface is visible
	const auto RtvHandle = SwapChain.GetRtvHandle(CurrentFrameIdx);
	CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);

	CommandList->SetGraphicsRootSignature(SceneLightPass.GetRootSignature());
	CommandList->SetPipelineState(SceneLightPass.GetPipelineState());

	// the G-buffer and the shadow map live in their own heap, so swap it in for the material one
	DXDescriptorHeap* LightHeaps[] = { LightInputsHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(LightHeaps), LightHeaps);

	CommandList->SetGraphicsRootDescriptorTable(SceneLightPass.GBufferTableIdx,	LightInputsHeap->GetGPUDescriptorHandleForHeapStart());
	CommandList->SetGraphicsRootConstantBufferView(SceneLightPass.CameraCBVIdx,	CameraCBV->GetGPUVirtualAddress() + (UINT64(CurrentFrameIdx) * CAMERA_CBV_STRIDE));
	CommandList->SetGraphicsRootConstantBufferView(SceneLightPass.LightsCBVIdx,	LightsCBV->GetGPUVirtualAddress() + (UINT64(CurrentFrameIdx) * LightsConstantBufferSize));

	SceneLightPass.RenderFullScreen(CommandList);
}

void D3D12Renderer::RenderDebugUI()
{
	// Straight over the shaded frame. LightPass already left the back buffer bound and
	// there is no depth, so the panel is simply drawn on top.
	SceneDebugUI.Draw(Camera, Catalog, Primitives, Lights, Materials, DefaultMaterial.get(), StepTimer.GetFramesPerSecond());
	SceneDebugUI.Render(FrameContext.GetCommandList());
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
}

void D3D12Renderer::UploadCameraConstants()
{
	DXASSERT(CameraCbvAddress, "Camera constant buffer must be mapped before uploading into it");

	CameraConstantBuffer CameraCbv	= {};
	CameraCbv.EyePosition			= Camera.GetPosition();

	// Undoing the view projection is what lets the sky turn a screen pixel back into a
	// world direction, transposed like every other matrix that goes to the GPU
	const auto ViewProj = Camera.GetViewMatrix() * Camera.GetProjMatrix();
	XMStoreFloat4x4(&CameraCbv.InvViewProj, XMMatrixTranspose(XMMatrixInverse(nullptr, ViewProj)));

	memcpy(CameraCbvAddress + (SIZE_T(CurrentFrameIdx) * CAMERA_CBV_STRIDE), &CameraCbv, sizeof(CameraCbv));
}

void D3D12Renderer::UploadLightConstants()
{
	DXASSERT(LightsCbvAddress, "Lights constant buffer must be mapped before uploading into it");

	LightsConstantBuffer LightsCbv	= {};
	LightsCbv.NumLights				= static_cast<UINT>(Lights.size() < MAX_LIGHTS ? Lights.size() : MAX_LIGHTS);

	for (UINT LightIdx = 0; LightIdx < LightsCbv.NumLights; ++LightIdx)
	{
		const auto& Light = *Lights[LightIdx];

		LightsCbv.Lights[LightIdx] = Light.GetLightConstants();

		// Every light casts, and the atlas has a slice per light of either kind, so the
		// light's own index is its slot. No allocator needed.
		LightsCbv.Lights[LightIdx].ShadowSlot = static_cast<INT>(LightIdx);

		// Only the single view kinds need a matrix here. A point light is looked up by
		// direction, so its six face matrices never leave the shadow pass.
		if (Light.GetLightType() != LightType::Point)
		{
			XMStoreFloat4x4(&LightsCbv.ShadowViewProj[LightIdx], XMMatrixTranspose(GetLightShadowViewProj(Light)));
		}
		else
		{
			XMStoreFloat4x4(&LightsCbv.ShadowViewProj[LightIdx], XMMatrixIdentity());
		}
	}

	memcpy(LightsCbvAddress + (SIZE_T(CurrentFrameIdx) * LightsConstantBufferSize), &LightsCbv, sizeof(LightsCbv));
}

void D3D12Renderer::UploadPrimitiveConstants(const Primitive& Primitive, const UINT PrimitiveIdx)
{
	DXASSERT(PrimitiveCbvAddress, "Primitive constant buffer must be mapped before uploading into it");

	// View and Proj mat per frame (camera can move every frame, so we need to update these every frame)
	const auto ViewMat = Camera.GetViewMatrix();
	const auto ProjMat = Camera.GetProjMatrix();

	const auto PrimitiveWorldMatrix	= Primitive.GetWorldMatrix();

	// WorldToClip (Remember to transpose for HLSL/GPU)
	const XMMATRIX LocalToWorld		= XMLoadFloat4x4(&PrimitiveWorldMatrix);
	const XMMATRIX WorldToClip		= XMLoadFloat4x4(&PrimitiveWorldMatrix) * ViewMat * ProjMat;

	PrimitiveConstantBuffer PrimitiveCbv = {};
	XMStoreFloat4x4(&PrimitiveCbv.LocalToWorld,	XMMatrixTranspose(LocalToWorld));
	XMStoreFloat4x4(&PrimitiveCbv.WorldToClip,	XMMatrixTranspose(WorldToClip));

	// Normal matrix. It is the inverse transpose of the world matrix, and everything here
	// goes to the GPU transposed, so the two transposes cancel and the plain inverse is
	// what gets stored. It looks like a missing transpose but it is not.
	XMStoreFloat4x4(&PrimitiveCbv.LocalToWorldNormal, XMMatrixInverse(nullptr, LocalToWorld));

	// No light matrix here any more. ShadowPass renders the same object from several
	// lights per frame, so it takes the light half as root constants instead.

	// The stride between frame regions is the capacity, not the current count. Using the
	// count would move every region the moment the panel adds a primitive, leaving the
	// frames still in flight reading from the wrong offsets.
	const SIZE_T FrameOffset = SIZE_T(CurrentFrameIdx) * MAX_PRIMITIVES * PrimitiveConstantBufferSize;
	memcpy(PrimitiveCbvAddress + FrameOffset + (SIZE_T(PrimitiveIdx) * PrimitiveConstantBufferSize), &PrimitiveCbv, PrimitiveConstantBufferSize);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Renderer::GetPrimitiveCbvAddress(const SIZE_T PrimitiveIdx) const
{
	DXASSERT(PrimitiveCBV, "Primitive constant buffer must be created before binding it");
	DXASSERT(PrimitiveIdx < Primitives.size(), "Primitive index is out of range");

	const UINT64 FrameOffset = UINT64(CurrentFrameIdx) * MAX_PRIMITIVES * PrimitiveConstantBufferSize;
	return PrimitiveCBV->GetGPUVirtualAddress() + FrameOffset + (UINT64(PrimitiveIdx) * PrimitiveConstantBufferSize);
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
