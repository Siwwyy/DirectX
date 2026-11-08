//Copyright, Damian Andrysiak 2026, All Rights Reserved.

// What the scene is made of, split out of D3D12Renderer.cpp.
//
// Everything here answers "what is in the world and what does it look like": which scene
// the command line asked for, the three scenes themselves, the lights they are lit by, and
// the factories that turn a folder of maps, a glTF file or a flat colour into a Material.
// D3D12Renderer.cpp keeps the part that answers "what happens each frame".
//
// The split is for reading rather than for building. These are still D3D12Renderer members
// and still touch its state directly, so nothing is encapsulated that was not before; the
// file simply stopped being long enough to get lost in.
//
// ProcessPendingSpawns deliberately stayed behind. It builds primitives much as the scenes
// below do, but it runs once a frame rather than once at startup, so it reads better next
// to the frame it belongs to.

#include "D3D12Renderer.h"

// Own Includes
#include "Paths.h"
#include "CommandLine.h"
#include "Model.h"
#include "GltfLoader.h"

SceneSetup D3D12Renderer::GetRequestedScene()
{
	// Shorthands first, since -sponza is what anyone actually types
	if (DXICMD::HasFlag(L"sponza"))		{ return SceneSetup::Sponza; }
	if (DXICMD::HasFlag(L"cornell"))	{ return SceneSetup::CornellBox; }

	const std::wstring Requested = DXICMD::GetString(L"scene", L"default");

	if (_wcsicmp(Requested.c_str(), L"default") == 0)	{ return SceneSetup::Default; }
	if (_wcsicmp(Requested.c_str(), L"sponza") == 0)	{ return SceneSetup::Sponza; }
	if (_wcsicmp(Requested.c_str(), L"both") == 0)		{ return SceneSetup::Both; }
	if (_wcsicmp(Requested.c_str(), L"cornell") == 0)	{ return SceneSetup::CornellBox; }

	DXLOG("Command line: unknown scene '%ls', using the built in one. Try -help.\n", Requested.c_str())
	return SceneSetup::Default;
}

void D3D12Renderer::InitializeScene()
{
	InitializeMaterials();

	// No sky. CubeMap still builds and the panorama is still in the content folder, but
	// nothing loads it, so there is no image based lighting and the background is flat.
	// Putting it back means restoring the two LoadRadiance/LoadIrradiance calls here, the
	// two SRVs in InitializePasses, and SKY_SRV_COUNT in LightPass.h.

	// Order matters when both are on. The built in scene hands materials out by index,
	// counting from the front of both lists, so it has to be built while those lists still
	// hold nothing but its own. Sponza appends 103 primitives and 25 materials and would
	// otherwise shift everything.
	if (UsesDefaultScene())
	{
		InitializeDefaultScene();
	}

	if (UsesSponzaScene())
	{
		InitializeSponzaScene();
	}

	if (UsesCornellBoxScene())
	{
		InitializeCornellBoxScene();
	}

	InitializeSceneLights();
}

void D3D12Renderer::InitializeDefaultScene()
{
	auto * const CommandList = FrameContext.GetCommandList();

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
	//
	// Skipped rather than fatal when absent: models are fetched by download_assets.bat
	// rather than committed, so a clone that has not been set up yet still runs, just
	// without the cow. Everything else in the scene is generated in code.
	{
		const auto SpotMesh = (DXIPATHS::GetModelDir / L"spot" / L"spot_triangulated.obj").Str();

		if (GetFileAttributesW(SpotMesh.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			Primitives.emplace_back(std::make_unique<Model>(TEXT("Spot"), SpotMesh));
		}
		else
		{
			DXLOG("Spot mesh is missing, run download_assets.bat to fetch it\n")
		}
	}

	// Last on purpose. The index based material hand out below counts from the front, so
	// anything added after the conditional model keeps its own index whether or not the
	// model was downloaded. Its material and transform are set by name further down.
	Primitives.emplace_back(std::make_unique<Sphere>(TEXT("Sphere"), 32));
	const SIZE_T SphereIdx = Primitives.size() - 1;

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

	// Sphere shares the metal plate maps with Cube4, which is worth seeing: the same
	// material on a flat face and on a curved one is where a normal map earns its keep.
	// Looked up by name rather than by index, since the sphere sits past the end of the
	// material list and the index hand out above leaves it on the default.
	Primitives[SphereIdx]->SetMaterial(FindMaterial(L"metal_plate"));
	Primitives[SphereIdx]->SetTransform({ -5.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, { 1.5f, 1.5f, 1.5f });

	// Cubes in a row, the last primitive is the ground laid out underneath them
	Primitives[0]->Transform({ -3.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
	Primitives[1]->Transform({ -1.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
	Primitives[2]->Transform({  1.0f, 1.0f, 1.0f }, DX_IDENTITY_ROTATE3, DX_IDENTITY_SCALE3);
	Primitives[3]->Transform({  0.0f, 0.0f, 3.0f }, { 90.f, 0.0f, 0.0f }, { 20.f, 20.f, 1.f });
	Primitives[4]->Transform({  10.0f, 0.0f, 10.0f }, { 90.f, 0.0f, 0.0f }, { 1.f, 1.f, 1.f });

	// Spot sits in front of the cubes. The mesh is modelled about a unit tall and lying
	// along +Z, so it is turned to face the camera and lifted clear of the ground.
	// Only present when the asset download has been run, hence the bounds check.
	if (Primitives.size() > 5)
	{
		Primitives[5]->Transform({  3.0f, 1.0f, 1.0f }, { 0.0f, -90.0f, 0.0f }, { 1.5f, 1.5f, 1.5f });
	}
}

void D3D12Renderer::InitializeSponzaScene()
{
	// Sponza, the Crytek atrium as redistributed by Khronos under CC-BY 3.0.
	//
	// Where it goes depends on whether it has the place to itself. On its own it sits at
	// the origin, which is where the camera starts and where the lights already point. In
	// Both it has to move: its floor is at y = 0, exactly where the built in ground plane
	// is, and the two would fight over every pixel they share. Standing it off along Z is
	// cheaper than arguing about depth bias.
	const XMFLOAT3 Placement = ActiveScene == SceneSetup::Both
								? XMFLOAT3(0.0f, 0.0f, 40.0f)
								: DX_IDENTITY_TRANSLATE3;

	LoadGltfScene((DXIPATHS::GetModelDir / L"sponza" / L"Sponza.gltf").Str(), Placement);

	// for redbull
	//LoadGltfScene((DXIPATHS::GetModelDir / L"redbull_rb22" / L"scene.gltf").Str(), Placement);
	//LoadGltfScene((DXIPATHS::GetModelDir / L"2018-renault-megane-rs-trophy-300" / L"scene.gltf").Str(), Placement);
}

void D3D12Renderer::InitializeCornellBoxScene()
{
	auto * const CommandList = FrameContext.GetCommandList();

	// The Cornell Box, to the measurements published at
	// https://www.graphics.cornell.edu/online/box/data.html
	//
	// A physically measured room, which is the point of it: the geometry and the surface
	// reflectances are known, so a render can be held against a photograph rather than
	// against an opinion. Five walls, two blocks, one light in the ceiling.
	//
	// Two deliberate departures from the published data. Cornell's units are roughly
	// millimetres, 552.8 across, which would dwarf everything else this engine draws, so
	// everything here is divided by 100 and recentred on the origin with the floor at y = 0.
	// And the room is mirrored on X, because Cornell's figures are right handed and ours
	// are not; without it the red and green walls come out swapped.
	constexpr float HalfWidth	= 2.764f;	// 552.8 / 2
	constexpr float Height		= 5.488f;	// 548.8
	constexpr float HalfDepth	= 2.796f;	// 559.2 / 2
	constexpr float Thickness	= 0.2f;		// Walls are slabs rather than quads, see below

	// Reflectances as the published spectra are usually reduced to RGB. Written straight in
	// as bytes because SetSolidColor fills an R8G8B8A8_UNORM texel, which the shader reads
	// linearly, so 186 means 0.73 rather than needing an sRGB curve undone first.
	constexpr BYTE White[4]	= { 186, 186, 186, 255 };	// 0.73
	constexpr BYTE Red[4]	= { 166,  13,  13, 255 };	// 0.65, 0.05, 0.05
	constexpr BYTE Green[4]	= {  31, 115,  38, 255 };	// 0.12, 0.45, 0.15

	// Matte paint, so fully rough. Metalness and specular keep their neutral defaults,
	// which is a non metal reflecting about 4 percent head on.
	constexpr BYTE FullyRough = 255;

	Material& WhiteMaterial	= CreateMaterialWithColor(L"Cornell White", White, FullyRough);
	Material& RedMaterial	= CreateMaterialWithColor(L"Cornell Red",	Red,   FullyRough);
	Material& GreenMaterial	= CreateMaterialWithColor(L"Cornell Green",	Green, FullyRough);

	// Walls are slabs rather than planes on purpose. A Plane is a single quad facing -Z
	// whose geometry sits half a unit off its own origin, so building a room out of them
	// means getting five rotations and five half unit offsets right, and any one of them
	// facing outwards leaves a wall that vanishes from inside. A flattened Cube has an
	// inward face whichever way it is turned, and is centred on its origin.
	const struct
	{
		const wchar_t*	Name;
		XMFLOAT3		Position;
		XMFLOAT3		Scale;
		Material*		SurfaceMaterial;
	}
	Walls[] =
	{
		// Inner surfaces land exactly on the room bounds, the thickness sits outside
		{ L"Cornell Floor",		{ 0.f, -Thickness * 0.5f, 0.f },			{ HalfWidth * 2.f + Thickness * 2.f, Thickness, HalfDepth * 2.f + Thickness * 2.f },	&WhiteMaterial },
		{ L"Cornell Ceiling",	{ 0.f, Height + Thickness * 0.5f, 0.f },	{ HalfWidth * 2.f + Thickness * 2.f, Thickness, HalfDepth * 2.f + Thickness * 2.f },	&WhiteMaterial },
		{ L"Cornell Back",		{ 0.f, Height * 0.5f, HalfDepth + Thickness * 0.5f },	{ HalfWidth * 2.f + Thickness * 2.f, Height, Thickness },				&WhiteMaterial },
		{ L"Cornell Left",		{ -(HalfWidth + Thickness * 0.5f), Height * 0.5f, 0.f },{ Thickness, Height, HalfDepth * 2.f },								&RedMaterial },
		{ L"Cornell Right",		{ HalfWidth + Thickness * 0.5f, Height * 0.5f, 0.f },	{ Thickness, Height, HalfDepth * 2.f },								&GreenMaterial },
	};

	// No front wall. The open side faces -Z, which is where the camera starts.
	for (const auto& Wall : Walls)
	{
		auto Slab = std::make_unique<Cube>(Wall.Name);
		Slab->Init(Device.Get(), CommandList);
		Slab->SetTransform(Wall.Position, DX_IDENTITY_ROTATE3, Wall.Scale);
		Slab->SetMaterial(Wall.SurfaceMaterial);
		Primitives.emplace_back(std::move(Slab));
	}

	// The two blocks. Centres, footprints and yaws were solved from the eight top face
	// corners in the published data, which come out square to within two millimetres.
	//
	// The yaw signs are not obvious and were checked rather than reasoned about: a row
	// vector through XMMatrixRotationY sends local +X to (cos, -sin) in world xz, so the
	// angle a corner list appears to make is the negative of the yaw that produces it.
	// Getting it backwards still gives a quad of the right size in the right place, just
	// turned the wrong way, which puts the corners 70cm out. These two are the signs whose
	// corners land on the published ones.
	const struct
	{
		const wchar_t*	Name;
		XMFLOAT3		Position;
		float			Yaw;
		float			Footprint;
		float			BlockHeight;
	}
	Blocks[] =
	{
		{ L"Cornell Short Block",	{  0.909f, 0.825f, -1.106f },	 17.0f, 1.67f, 1.65f },
		{ L"Cornell Tall Block",	{ -0.921f, 1.650f,  0.717f },	-17.1f, 1.66f, 3.30f },
	};

	for (const auto& Block : Blocks)
	{
		auto Solid = std::make_unique<Cube>(Block.Name);
		Solid->Init(Device.Get(), CommandList);
		Solid->SetTransform(Block.Position, { 0.f, Block.Yaw, 0.f }, { Block.Footprint, Block.BlockHeight, Block.Footprint });
		Solid->SetMaterial(&WhiteMaterial);
		Primitives.emplace_back(std::move(Solid));
	}
}

void D3D12Renderer::InitializeSceneLights()
{
	// The Cornell Box is lit by one quad in the ceiling and nothing else. A sun would
	// defeat the point, since the room is closed and the whole exercise is what a single
	// known emitter does to it.
	if (UsesCornellBoxScene())
	{
		// Position and size from the published light quad, 130 by 105 units across, hung
		// just under the ceiling so the slab above does not occlude it
		auto Ceiling = std::make_unique<AreaLight>(TEXT("Cornell Light"));
		Ceiling->SetPosition({ -0.016f, 5.44f, -0.001f });
		Ceiling->SetDirection({ 0.0f, -1.0f, 0.0f });
		Ceiling->SetSize({ 1.30f, 1.05f });
		Ceiling->SetColor({ 1.0f, 0.9f, 0.75f });

		// Intensity is divided by distance squared and the floor is five and a half units
		// down, so this is far larger than the numbers the built in scene uses. A starting
		// point, tune it in the panel.
		Ceiling->SetIntensity(60.0f);
		Ceiling->SetRange(14.0f);
		Lights.emplace_back(std::move(Ceiling));

		DXASSERT(Lights.size() <= MAX_LIGHTS, "More lights placed in the scene than the constant buffer can hold");
		return;
	}

	/***************************
	 ********** Lights *********
	 ***************************/
	{
		// Sun above the scene, shining down from the left and behind
		auto Sun = std::make_unique<DirectionalLight>(TEXT("DirectionalLight1"));
		Sun->SetDirection({ -0.5f, -1.0f, 0.5f });
		Sun->SetColor({ 1.0f, 0.95f, 0.8f });
		Sun->SetIntensity(0.6f);

		// The shadow box has to contain everything that should cast, and there is only one
		// of it, so it is sized to whichever scene is up.
		//
		// A wider box is a blurrier one: the extent spreads over a fixed 2048 texels either
		// way. Covering the built in scene and Sponza at once would need roughly 70 units
		// against the 20 the cubes want, which is three and a half times the texel size for
		// the sake of a comparison mode. Both therefore keeps the tight box and leaves
		// Sponza unshadowed by the sun; pick -scene sponza to have it shadowed properly.
		if (ActiveScene == SceneSetup::Sponza)
		{
			// Sponza reaches 15 units either side of the origin and 11 up, so the box is
			// sized to the width and the camera walked far enough back to clear the roof
			Sun->SetShadowBounds({ 0.0f, 2.0f, 0.0f }, 34.f, 40.f);
		}
		else
		{
			// The cubes sit around the origin and the ground reaches a few units out
			Sun->SetShadowBounds({ 0.0f, 0.0f, 1.0f }, 20.f, 20.f);
		}

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

	// Only what the built in scene asks for. Sponza brings its own twenty five, and these
	// six are around 150MB of decoded maps that a Sponza only run would never sample.
	// The panel can still load any of them on demand, through the catalog scanned below.
	if (UsesDefaultScene())
	{
		// All CC0 from Poly Haven, in the order the primitives are created below, since
		// materials are handed out by index.
		LoadMaterial(L"american_walnut_veneer");	// Cube1
		LoadMaterial(L"brick_4");					// Cube2
		LoadMaterial(L"brushed_concrete");			// Cube3

		// The ground is a single quad with no thickness, so it has to survive being looked
		// at from underneath
		LoadMaterial(L"blue_floor_tiles_01").SetTwoSided(true);		// Ground

		LoadMaterial(L"metal_plate");				// Cube4

		// Spot ships one colour map and nothing else, so the remaining slots fall back to
		// the neutral values SetDefaults fills in. Downloaded alongside the mesh, so it is
		// only created when the asset is actually here.
		const auto SpotTexture = (DXIPATHS::GetModelDir / L"spot" / L"spot_texture.png").Str();

		if (GetFileAttributesW(SpotTexture.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			CreateMaterialWithDiffuse(L"spot", SpotTexture);	// Spot
		}
	}

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

	DXASSERT(!UsesDefaultScene() || FindMaterial(L"metal_plate"), "The built in scene asks for metal_plate by name, it has to be among the loaded materials");
}

Material* D3D12Renderer::FindMaterial(const std::wstring& Name) const
{
	for (const auto& Candidate : Materials)
	{
		if (Candidate->GetName() == Name)
		{
			return Candidate.get();
		}
	}

	// Null means "use the default", which is what Primitive::SetMaterial takes it as
	return nullptr;
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

void D3D12Renderer::LoadGltfScene(const std::wstring& Path, const XMFLOAT3 Translation, const XMFLOAT3 Rotation, const XMFLOAT3 Scale)
{
	if (GetFileAttributesW(Path.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		DXLOG("glTF scene is missing, run download_assets.bat to fetch it: %ls\n", Path.c_str())
		return;
	}

	auto * const CommandList	= FrameContext.GetCommandList();
	const GltfScene Scene		= LoadGltfFromFile(Path);

	// glTF numbers its materials per file, we number ours per scene, so a primitive's
	// material index has to be translated before it means anything here. An entry stays -1
	// when the material could not be created, which leaves that primitive on the default.
	std::vector<int> MaterialRemap(Scene.Materials.size(), -1);

	for (SIZE_T Idx = 0; Idx < Scene.Materials.size(); ++Idx)
	{
		if (Materials.size() >= MAX_MATERIALS)
		{
			DXLOG("glTF: material heap is full at %u, the rest of the scene falls back to the default material\n", MAX_MATERIALS)
			break;
		}

		const GltfMaterial& Source = Scene.Materials[Idx];

		auto NewMaterial = std::make_unique<Material>(Source.Name);
		NewMaterial->Init(Device.Get(), SharedMaterialHeap);

		if (!Source.BaseColorPath.empty())
		{
			NewMaterial->LoadTexture(Device.Get(), CommandList, MaterialTextureSlot::Diffuse, Source.BaseColorPath);
		}

		if (!Source.NormalPath.empty())
		{
			NewMaterial->LoadTexture(Device.Get(), CommandList, MaterialTextureSlot::Normal, Source.NormalPath);
		}

		// Roughness and metalness are one image in glTF, packed into the green and blue
		// channels, where we keep a slot for each and read the red channel of both. Loading
		// it into either slot would therefore read the wrong channel, so it is left out and
		// both slots take the neutral values below. Fixing it properly means either
		// splitting the image on load or telling the shader which convention it is reading,
		// and neither matters yet: nothing samples those two slots.
		// Source.MetallicRoughnessPath holds the path for whichever way that goes.

		// Whatever was not supplied gets a neutral value rather than a null descriptor
		NewMaterial->SetDefaults(Device.Get(), CommandList);
		NewMaterial->SetTwoSided(Source.bDoubleSided);

		MaterialRemap[Idx] = static_cast<int>(Materials.size());
		Materials.emplace_back(std::move(NewMaterial));
	}

	SIZE_T Added = 0;

	for (const GltfPrimitive& Source : Scene.Primitives)
	{
		if (Primitives.size() >= MAX_PRIMITIVES)
		{
			DXLOG("glTF: primitive ceiling of %u reached, %u pieces of the scene were dropped\n",
					MAX_PRIMITIVES, static_cast<UINT>(Scene.Primitives.size() - Added))
			break;
		}

		// Copied rather than moved, because the scene is const here and a primitive's
		// geometry is wanted by value anyway
		auto Spawned = std::make_unique<Model>(Source.Name, Source.Mesh);
		Spawned->Init(Device.Get(), CommandList);

		// Every piece takes the same transform. The loader has already put the vertices in
		// the file's own world space, so this moves the scene as a whole rather than
		// placing a piece within it.
		Spawned->SetTransform(Translation, Rotation, Scale);

		if (Source.MaterialIdx >= 0 && MaterialRemap[Source.MaterialIdx] >= 0)
		{
			Spawned->SetMaterial(Materials[MaterialRemap[Source.MaterialIdx]].get());
		}

		Primitives.emplace_back(std::move(Spawned));
		++Added;
	}

	DXLOG("glTF: added %u primitives, the scene now holds %u of %u\n",
			static_cast<UINT>(Added), static_cast<UINT>(Primitives.size()), MAX_PRIMITIVES)
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

Material& D3D12Renderer::CreateMaterialWithColor(const wchar_t* Name, const BYTE (&Rgba)[4], const BYTE Roughness)
{
	DXASSERT(Name, "Material needs a name");
	DXASSERT(Materials.size() < MAX_MATERIALS, "More materials than the shared heap can hold");

	auto NewMaterial = std::make_unique<Material>(Name);
	NewMaterial->Init(Device.Get(), SharedMaterialHeap);

	// Before SetDefaults, which only fills slots that are still empty
	const BYTE RoughnessRgba[4] = { Roughness, Roughness, Roughness, 255 };
	NewMaterial->SetSolidColor(Device.Get(), FrameContext.GetCommandList(), MaterialTextureSlot::Diffuse,	Rgba);
	NewMaterial->SetSolidColor(Device.Get(), FrameContext.GetCommandList(), MaterialTextureSlot::Roughness,	RoughnessRgba);

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
