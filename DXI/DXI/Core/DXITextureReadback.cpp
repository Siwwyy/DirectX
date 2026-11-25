//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "DXITextureReadback.h"

#include <DirectXPackedVector.h>

// Namespaces
using namespace Helpers;

namespace
{
	// Bytes one pixel takes in a format this decodes, 0 for any other
	UINT GetBytesPerPixel(const DXGI_FORMAT Format) noexcept
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return 4;
		case DXGI_FORMAT_R16G16_FLOAT:			return 4;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return 8;
		case DXGI_FORMAT_R32_FLOAT:				return 4;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return 16;
		default:								return 0;
		}
	}
}

UINT DXITextureReadback::GetChannelCount(const DXGI_FORMAT Format) noexcept
{
	switch (Format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:		return 4;
	case DXGI_FORMAT_R16G16_FLOAT:			return 2;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:	return 4;
	case DXGI_FORMAT_R32_FLOAT:				return 1;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:	return 4;
	default:								return 0;
	}
}

UINT DXITextureReadback::GetChannelCount() const noexcept
{
	return GetChannelCount(Format);
}

void DXITextureReadback::RecordCopy(DXDevice * Device,
									DXGraphicsCommandList * CommandList,
									DXResource * Source,
									const DXGI_FORMAT CopyFormat,
									const D3D12_RESOURCE_STATES SourceState)
{
	DXASSERT(Device && CommandList && Source, "A readback needs a device, a command list and a texture");
	DXASSERT(GetBytesPerPixel(CopyFormat) > 0, "Readback does not know how to decode this format, add it to DXITextureReadback");

	// How the copy lays the rows out in the buffer. Each row starts on a 256 byte boundary,
	// so the pitch is usually wider than the pixels in it, and decoding has to step by it.
	D3D12_RESOURCE_DESC SourceDesc	= Source->GetDesc();
	DXASSERT(SourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D, "Readback copies 2D textures");
	SourceDesc.Format				= CopyFormat;

	UINT64 TotalBytes	= 0;
	UINT   NumRows		= 0;
	UINT64 RowBytes		= 0;
	Device->GetCopyableFootprints(&SourceDesc, 0, 1, 0, &Layout, &NumRows, &RowBytes, &TotalBytes);

	BufferBytes	= TotalBytes;
	Width	= static_cast<UINT>(SourceDesc.Width);
	Height	= SourceDesc.Height;
	Format	= CopyFormat;

	// The buffer the copy lands in, a readback heap so the CPU can map it. It starts as a
	// copy destination, which is the only state a readback heap buffer can ever be in.
	const auto BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(TotalBytes);
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_READBACK,
		D3D12_HEAP_FLAG_NONE,
		&BufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&Buffer)));
	Buffer->SetName(L"Readback Buffer");

	// Into the state a copy reads from, unless it is there already
	const bool bNeedsBarrier = SourceState != D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (bNeedsBarrier)
	{
		const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Source, SourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CommandList->ResourceBarrier(1, &Barrier);
	}

	const CD3DX12_TEXTURE_COPY_LOCATION Destination(Buffer.Get(), Layout);
	const CD3DX12_TEXTURE_COPY_LOCATION SourceLocation(Source, 0);
	CommandList->CopyTextureRegion(&Destination, 0, 0, 0, &SourceLocation, nullptr);

	// And back, so the passes after this one find the texture the way they left it
	if (bNeedsBarrier)
	{
		const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Source, D3D12_RESOURCE_STATE_COPY_SOURCE, SourceState);
		CommandList->ResourceBarrier(1, &Barrier);
	}
}

bool DXITextureReadback::Read(std::vector<float>& OutPixels) const
{
	const UINT Channels			= GetChannelCount(Format);
	const UINT BytesPerPixel	= GetBytesPerPixel(Format);
	if (!Buffer || Channels == 0 || BytesPerPixel == 0)
	{
		return false;
	}

	// The whole buffer is what the copy wrote. The range must not run past the end of the
	// resource, and RowPitch * Height does: the last row is only as long as its pixels, not
	// padded out to the pitch, so the size the footprint reported is used as it is.
	const D3D12_RANGE ReadRange = { 0, static_cast<SIZE_T>(BufferBytes) };

	uint8_t * Mapped = nullptr;
	if (FAILED(Buffer->Map(0, &ReadRange, reinterpret_cast<void**>(&Mapped))) || !Mapped)
	{
		return false;
	}

	OutPixels.resize(static_cast<size_t>(Width) * Height * Channels);

	for (UINT Y = 0; Y < Height; ++Y)
	{
		const uint8_t * Row		= Mapped + Layout.Offset + (static_cast<size_t>(Layout.Footprint.RowPitch) * Y);
		float * const OutRow	= OutPixels.data() + (static_cast<size_t>(Y) * Width * Channels);
		const size_t RowValues	= static_cast<size_t>(Width) * Channels;

		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			// A byte is a number from 0 to 255, and unorm means that is 0 to 1
			for (size_t Idx = 0; Idx < RowValues; ++Idx)
			{
				OutRow[Idx] = static_cast<float>(Row[Idx]) / 255.0f;
			}
			break;

		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			{
				const auto * const Halves = reinterpret_cast<const DirectX::PackedVector::HALF*>(Row);
				for (size_t Idx = 0; Idx < RowValues; ++Idx)
				{
					OutRow[Idx] = DirectX::PackedVector::XMConvertHalfToFloat(Halves[Idx]);
				}
			}
			break;

		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			memcpy(OutRow, Row, RowValues * sizeof(float));
			break;

		default:
			Buffer->Unmap(0, nullptr);
			return false;
		}
	}

	// Nothing was written through the mapping, so the written range is empty
	const D3D12_RANGE WrittenRange = { 0, 0 };
	Buffer->Unmap(0, &WrittenRange);

	return true;
}
