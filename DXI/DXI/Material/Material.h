//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_MATERIAL_H_INCLUDED
#define D3D12_MATERIAL_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXITexture.h"

// Texture slots of a single material, one descriptor each.
// The order here is the register order the pixel shader sees, i.e., Diffuse lands on t0, Normal on t1 and so on.
enum class MaterialTextureSlot : UINT
{
	Diffuse = 0,
	Normal,
	Specular,
	Metalness,
	Roughness,
	AmbientOcclusion,

	Count
};

// How many descriptors a single material takes out of the shared heap
constexpr UINT MaterialTextureSlotCount = static_cast<UINT>(MaterialTextureSlot::Count);

// Slot name, for resource naming in PIX and for asserts
_NODISCARD const wchar_t* ToString(const MaterialTextureSlot Slot);


// One shader visible CBV/SRV/UAV heap shared by every material.
// Material N owns the descriptor range [N * MaterialTextureSlotCount, (N + 1) * MaterialTextureSlotCount),
// so binding a material is a single SetGraphicsRootDescriptorTable with the base handle of its range.
class MaterialHeap
{
	DISABLE_COPY_MOVE(MaterialHeap)

public:

	MaterialHeap()	= default;
	~MaterialHeap() = default;

	// Creates the heap with room for MaxMaterials materials
	void Init(DXDevice * Device, const UINT MaxMaterials);

	// Reserves the descriptor range of one material and returns its index
	_NODISCARD UINT Allocate();

	// CPU handle of a single slot, this is where a texture writes its SRV
	_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const UINT MaterialIdx, const MaterialTextureSlot Slot) const;

	// GPU handle of the first slot of a material, this is the descriptor table to bind
	_NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const UINT MaterialIdx) const;

	// Getters
	_NODISCARD FORCEINLINE DXDescriptorHeap* GetHeap() const
	{
		return Heap.Get();
	}
	_NODISCARD FORCEINLINE UINT GetCapacity() const
	{
		return Capacity;
	}
	_NODISCARD FORCEINLINE UINT GetNumAllocated() const
	{
		return NumAllocated;
	}

private:

	// Flat descriptor index of a slot inside the heap
	_NODISCARD UINT GetDescriptorIdx(const UINT MaterialIdx, const MaterialTextureSlot Slot) const;

	ComPtr<DXDescriptorHeap>	Heap;							// the one big heap every material lives in
	UINT						IncrementDescriptorSize = 0;	// size of a single descriptor on this device
	UINT						Capacity				= 0;	// in materials, not in descriptors
	UINT						NumAllocated			= 0;	// in materials, not in descriptors
};


// Set of textures describing a single surface.
// The textures themselves are owned here, their descriptors live in the shared MaterialHeap.
class Material
{
	DISABLE_COPY(Material)

public:

	Material()	= default;
	Material(std::wstring Name);
	~Material()	= default;

	// Reserves this material's range in the shared heap and points every slot at a null descriptor
	void Init(DXDevice * Device, MaterialHeap& InMaterialHeap);

	// Loads a texture from disk into one of the slots, overwriting its null descriptor
	void LoadTexture(DXDevice * Device, DXGraphicsCommandList * CommandList, const MaterialTextureSlot Slot, std::wstring TexturePath);

	// Fills a slot with a single colour instead of an image, which is how a material
	// gets sensible values for a map it has no texture for
	void SetSolidColor(DXDevice * Device, DXGraphicsCommandList * CommandList, const MaterialTextureSlot Slot, const BYTE (&Rgba)[4]);

	// Gives every slot a neutral value: mid grey surface, flat normal, no metal, half
	// rough and unoccluded. A mesh drawn with this is visible and obviously untextured
	// rather than black.
	void SetDefaults(DXDevice * Device, DXGraphicsCommandList * CommandList);

	// Descriptor table to bind for this material, covers every slot at once
	_NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorTable() const;

	// Drops the staging copies of the textures.
	//
	// A texture is uploaded by writing it into an upload heap and copying from there into
	// the default heap the shader reads, and the upload heap has to outlive the copy, which
	// is why it is held at all. Once the command list carrying that copy has been waited
	// on, it is dead weight: an exact duplicate of every texture, sitting in system memory
	// for as long as the material does.
	//
	// Six cubes never made that worth noticing. A scene arriving with twenty five materials
	// of 1k maps does: the duplicates come to a few hundred megabytes.
	//
	// Only safe after the GPU has finished the copies, so call it past a flush.
	void ReleaseUploadHeaps();

	// Setters
	// Two sided surfaces keep their back faces, which a single quad like a ground plane
	// needs to stay visible from underneath. Culling lives in the pipeline state, so this
	// makes the pass pick its two sided variant instead.
	void SetTwoSided(const bool bTwoSided) noexcept
	{
		this->bTwoSided = bTwoSided;
	}

	// Getters
	_NODISCARD FORCEINLINE bool IsTwoSided() const noexcept
	{
		return bTwoSided;
	}
	_NODISCARD FORCEINLINE bool IsInitialized() const
	{
		return Heap != nullptr;
	}
	_NODISCARD FORCEINLINE UINT GetMaterialIdx() const
	{
		return MaterialIdx;
	}
	_NODISCARD FORCEINLINE const std::wstring& GetName() const
	{
		return Name;
	}
	_NODISCARD FORCEINLINE bool HasTexture(const MaterialTextureSlot Slot) const
	{
		return TextureBuffers[static_cast<UINT>(Slot)] != nullptr;
	}

private:

	// Creates the slot's texture, copies the given pixels into it and points the slot's
	// descriptor at the result. Shared by both ways a slot can be filled.
	void UploadSlotTexture(DXDevice * Device,
							DXGraphicsCommandList * CommandList,
							const MaterialTextureSlot Slot,
							const D3D12_RESOURCE_DESC& TextureDesc,
							const void * Pixels,
							const SIZE_T RowPitch);

	// Shared storage of the descriptors, not owned by the material
	MaterialHeap *				Heap			= nullptr;

	// Index of this material inside the shared heap
	UINT						MaterialIdx		= 0;

	// Texture resources per slot, an empty slot keeps a null descriptor so the shader always reads something valid
	ComPtr<DXResource>			TextureBuffers[MaterialTextureSlotCount];		// default heap resources holding the images
	ComPtr<DXResource>			TextureUploadHeaps[MaterialTextureSlotCount];	// upload heaps the images are copied from

	// Name of material (for debug purposes)
	std::wstring				Name			= L"Material";

	// Whether back faces survive the rasterizer
	bool						bTwoSided		= false;
};

#endif // D3D12_MATERIAL_H_INCLUDED
