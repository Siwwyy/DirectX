//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "Material.h"

// Namespaces
using namespace Helpers;

// Format the null descriptors are created with. Sampling an unbound slot returns zeros,
// so a material without e.g. a normal map is still safe to read in the shader.
static constexpr DXGI_FORMAT NULL_DESCRIPTOR_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

const wchar_t* ToString(const MaterialTextureSlot Slot)
{
	switch (Slot)
	{
	case MaterialTextureSlot::Diffuse:			return L"Diffuse";
	case MaterialTextureSlot::Normal:			return L"Normal";
	case MaterialTextureSlot::Specular:			return L"Specular";
	case MaterialTextureSlot::Metalness:		return L"Metalness";
	case MaterialTextureSlot::Roughness:		return L"Roughness";
	case MaterialTextureSlot::AmbientOcclusion:	return L"AmbientOcclusion";
	default:									return L"Unknown";
	}
}


// Material Heap class
void MaterialHeap::Initialize(DXDevice * Device, const UINT MaxMaterials)
{
	DXASSERT(Device, "Device can not be null for Initialization of the material heap");
	DXASSERT(MaxMaterials > 0, "Material heap needs room for at least one material");

	constexpr auto DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	// One big heap for every material, CBV/SRV/UAV so constants and UAVs can share it later on
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.NumDescriptors	= MaxMaterials * MaterialTextureSlotCount;
	HeapDesc.Type			= DescriptorHeapType;
	HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap)));
	NAME_D3D12_OBJECT(Heap, L"Material Descriptor Heap");

	IncrementDescriptorSize	= Device->GetDescriptorHandleIncrementSize(DescriptorHeapType);
	Capacity				= MaxMaterials;
	NumAllocated			= 0;
}

UINT MaterialHeap::Allocate()
{
	DXASSERT(Heap, "Material heap must be initialized before allocating a material");
	DXASSERT(NumAllocated < Capacity, "Material heap is out of space, initialize it with a bigger MaxMaterials");

	const UINT CurrentAllocationIdx = NumAllocated;
	NumAllocated = NumAllocated + 1;
	return CurrentAllocationIdx;
}

D3D12_CPU_DESCRIPTOR_HANDLE MaterialHeap::GetCPUHandle(const UINT MaterialIdx, const MaterialTextureSlot Slot) const
{
	DXASSERT(Heap, "Material heap must be initialized before taking a handle out of it");

	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(Heap->GetCPUDescriptorHandleForHeapStart());
	Handle.Offset(GetDescriptorIdx(MaterialIdx, Slot), IncrementDescriptorSize);
	return Handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE MaterialHeap::GetGPUHandle(const UINT MaterialIdx) const
{
	DXASSERT(Heap, "Material heap must be initialized before taking a handle out of it");

	// Table starts at the first slot of the material
	CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(Heap->GetGPUDescriptorHandleForHeapStart());
	Handle.Offset(GetDescriptorIdx(MaterialIdx, MaterialTextureSlot::Diffuse), IncrementDescriptorSize);
	return Handle;
}

UINT MaterialHeap::GetDescriptorIdx(const UINT MaterialIdx, const MaterialTextureSlot Slot) const
{
	DXASSERT(MaterialIdx < NumAllocated, "Material index is outside of the allocated range of the heap");
	DXASSERT(Slot < MaterialTextureSlot::Count, "Unknown material texture slot");

	return (MaterialIdx * MaterialTextureSlotCount) + static_cast<UINT>(Slot);
}


// Material class
Material::Material(std::wstring Name)
	: Name(std::move(Name))
{
}

void Material::Initialize(DXDevice * Device, MaterialHeap& InMaterialHeap)
{
	DXASSERT(Device, "Device can not be null for Initialization of the material");
	DXASSERT(!IsInitialized(), "Material has been already initialized");

	Heap		= &InMaterialHeap;
	MaterialIdx	= InMaterialHeap.Allocate();

	// Every descriptor of the range has to be written before the table is bound, so start the whole
	// range as null descriptors and let LoadTexture() replace the slots that actually have an image
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format					= NULL_DESCRIPTOR_FORMAT;
	SrvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels		= 1;

	for (UINT SlotIdx = 0; SlotIdx < MaterialTextureSlotCount; ++SlotIdx)
	{
		const auto Slot = static_cast<MaterialTextureSlot>(SlotIdx);
		Device->CreateShaderResourceView(nullptr, &SrvDesc, Heap->GetCPUHandle(MaterialIdx, Slot));
	}
}

void Material::LoadTexture(DXDevice * Device, DXGraphicsCommandList * CommandList, const MaterialTextureSlot Slot, std::wstring TexturePath)
{
	DXASSERT(Device && CommandList, "Device and command list can not be null for loading a material texture");
	DXASSERT(IsInitialized(), "Material must be initialized before loading its textures");
	DXASSERT(Slot < MaterialTextureSlot::Count, "Unknown material texture slot");
	DXASSERT(!HasTexture(Slot), "Material slot has been already loaded");

	// Decode the image off disk
	D3D12_RESOURCE_DESC TextureDesc;
	int ImageBytesPerRow = 0;
	SimpleSmartPointer<BYTE> ImageData = nullptr;
	LoadImageDataFromFile(ImageData, TextureDesc, TexturePath.c_str(), ImageBytesPerRow);

	UploadSlotTexture(Device, CommandList, Slot, TextureDesc, ImageData.Get(), static_cast<SIZE_T>(ImageBytesPerRow));
}

void Material::SetSolidColor(DXDevice * Device, DXGraphicsCommandList * CommandList, const MaterialTextureSlot Slot, const BYTE (&Rgba)[4])
{
	DXASSERT(Device && CommandList, "Device and command list can not be null for filling a material slot");
	DXASSERT(IsInitialized(), "Material must be initialized before filling its slots");
	DXASSERT(Slot < MaterialTextureSlot::Count, "Unknown material texture slot");
	DXASSERT(!HasTexture(Slot), "Material slot has been already loaded");

	// A single texel is enough, every uv lands on the same colour anyway
	const auto TextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);

	UploadSlotTexture(Device, CommandList, Slot, TextureDesc, Rgba, sizeof(Rgba));
}

void Material::SetDefaults(DXDevice * Device, DXGraphicsCommandList * CommandList)
{
	// Neutral values for everything, so a mesh with no material of its own still shades
	// correctly instead of coming out black or mirror smooth
	BYTE Diffuse[4]		= { 204, 204, 204, 255 };	// 0.8 grey, obviously untextured
	BYTE Normal[4]		= { 128, 128, 255, 255 };	// flat, pointing straight out of the surface
	BYTE Specular[4]	= {  10,  10,  10, 255 };	// about 0.04, what a dielectric reflects head on
	BYTE Metalness[4]	= {   0,   0,   0, 255 };	// not a metal
	BYTE Roughness[4]	= { 128, 128, 128, 255 };	// half rough, neither mirror nor fully diffuse
	BYTE Occlusion[4]	= { 255, 255, 255, 255 };	// nothing occluded

	SetSolidColor(Device, CommandList, MaterialTextureSlot::Diffuse,			Diffuse);
	SetSolidColor(Device, CommandList, MaterialTextureSlot::Normal,				Normal);
	SetSolidColor(Device, CommandList, MaterialTextureSlot::Specular,			Specular);
	SetSolidColor(Device, CommandList, MaterialTextureSlot::Metalness,			Metalness);
	SetSolidColor(Device, CommandList, MaterialTextureSlot::Roughness,			Roughness);
	SetSolidColor(Device, CommandList, MaterialTextureSlot::AmbientOcclusion,	Occlusion);
}

void Material::UploadSlotTexture(DXDevice * Device,
								DXGraphicsCommandList * CommandList,
								const MaterialTextureSlot Slot,
								const D3D12_RESOURCE_DESC& TextureDesc,
								const void * Pixels,
								const SIZE_T RowPitch)
{
	const auto SlotIdx	= static_cast<UINT>(Slot);
	auto& TextureBuffer	= TextureBuffers[SlotIdx];
	auto& UploadHeap	= TextureUploadHeaps[SlotIdx];

	const auto Width	= static_cast<UINT>(TextureDesc.Width);
	const auto Height	= TextureDesc.Height;

	// Box filter the image down the whole way. Without mips a texture is sampled at full
	// resolution however few pixels it covers, which is what makes a surface shimmer in
	// the distance, and it is exactly what the mip debug view of BasePass reports.
	const auto Mips			= GenerateMipChain(static_cast<const BYTE*>(Pixels), Width, Height, static_cast<UINT>(RowPitch), TextureDesc.Format);
	const auto MipCount		= static_cast<UINT>(Mips.size());

	// Texel size comes from the format, a grayscale map is one byte and not four.
	// Asking the format rather than dividing the row pitch keeps this agreeing with the
	// mip generator even if the pitch ever carries padding.
	const auto BytesPerTexel = GetFormatBytesPerTexel(TextureDesc.Format);
	DXASSERT(BytesPerTexel > 0, "Material slot has a format the texel size is unknown for");

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	auto MippedDesc			= TextureDesc;
	MippedDesc.MipLevels	= static_cast<UINT16>(MipCount);

	constexpr auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
	constexpr auto StateAfter	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT,	// a default heap
		D3D12_HEAP_FLAG_NONE,		// no flags
		&MippedDesc,				// the description of our texture
		StateBefore,				// We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr,					// used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&TextureBuffer)));
	TextureBuffer->SetName((Name + L" " + ToString(Slot)).c_str());

	// now we create an upload heap big enough for every mip at once
	const UINT64 TextureUploadBufferSize = GetRequiredIntermediateSize(TextureBuffer.Get(), 0, MipCount);
	const auto TextureBufferUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(TextureUploadBufferSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_UPLOAD,			// upload heap
		D3D12_HEAP_FLAG_NONE,				// no flags
		&TextureBufferUploadDesc,			// resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ,	// We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&UploadHeap)));
	UploadHeap->SetName((Name + L" " + ToString(Slot) + L" Upload Resource Heap").c_str());

	// store every mip in the upload heap
	std::vector<D3D12_SUBRESOURCE_DATA> Subresources(MipCount);

	UINT MipWidth	= Width;
	UINT MipHeight	= Height;

	for (UINT MipIdx = 0; MipIdx < MipCount; ++MipIdx)
	{
		const SIZE_T MipPitch			= SIZE_T(MipWidth) * BytesPerTexel;

		Subresources[MipIdx].pData		= Mips[MipIdx].data();
		Subresources[MipIdx].RowPitch	= MipPitch;
		Subresources[MipIdx].SlicePitch	= MipPitch * MipHeight;

		MipWidth	= MipWidth > 1 ? MipWidth / 2 : 1;
		MipHeight	= MipHeight > 1 ? MipHeight / 2 : 1;
	}

	// Now we copy the upload buffer contents to the default heap
	UpdateSubresources(CommandList, TextureBuffer.Get(), UploadHeap.Get(), 0, 0, MipCount, Subresources.data());

	// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
	const auto Transition = CD3DX12_RESOURCE_BARRIER::Transition(TextureBuffer.Get(), StateBefore, StateAfter);
	CommandList->ResourceBarrier(1, &Transition);

	// now we create a shader resource view over this material's slot, replacing its null descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format					= TextureDesc.Format;
	SrvDesc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels		= MipCount;
	Device->CreateShaderResourceView(TextureBuffer.Get(), &SrvDesc, Heap->GetCPUHandle(MaterialIdx, Slot));
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetDescriptorTable() const
{
	DXASSERT(IsInitialized(), "Material must be initialized before binding it");

	return Heap->GetGPUHandle(MaterialIdx);
}
