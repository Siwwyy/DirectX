//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "CubeMap.h"

#include <cmath>

// Namespaces
using namespace Helpers;

namespace
{
	// Bytes per texel of both the panorama and the faces
	constexpr UINT BYTES_PER_TEXEL = 4;

	// Hemisphere samples taken per irradiance texel, in each of the two angles
	constexpr UINT IRRADIANCE_PHI_SAMPLES	= 24;
	constexpr UINT IRRADIANCE_THETA_SAMPLES	= 12;

	// Direction a face texel points at, in the D3D face order.
	// Uc and Vc are the texel position inside the face mapped to [-1,1], with Vc pointing up.
	XMVECTOR GetFaceDirection(const UINT FaceIdx, const float Uc, const float Vc)
	{
		switch (FaceIdx)
		{
		case 0:		return XMVectorSet( 1.f,  Vc, -Uc, 0.f);	// +X
		case 1:		return XMVectorSet(-1.f,  Vc,  Uc, 0.f);	// -X
		case 2:		return XMVectorSet( Uc,  1.f, -Vc, 0.f);	// +Y
		case 3:		return XMVectorSet( Uc, -1.f,  Vc, 0.f);	// -Y
		case 4:		return XMVectorSet( Uc,  Vc,  1.f, 0.f);	// +Z
		default:	return XMVectorSet(-Uc,  Vc, -1.f, 0.f);	// -Z
		}
	}

	// Where a direction lands in an equirectangular panorama
	void DirectionToEquirectangular(const XMVECTOR Direction, float& OutU, float& OutV)
	{
		const float Dx = XMVectorGetX(Direction);
		const float Dy = XMVectorGetY(Direction);
		const float Dz = XMVectorGetZ(Direction);

		// Longitude around the Y axis, latitude from the +Y pole down
		OutU = (atan2f(Dz, Dx) / XM_2PI) + 0.5f;
		OutV = acosf(Dy < -1.f ? -1.f : (Dy > 1.f ? 1.f : Dy)) / XM_PI;
	}

	// Bilinear fetch out of the panorama. U wraps around because a panorama is seamless
	// left to right, V clamps because the poles are not.
	void SampleEquirectangular(const BYTE * Source,
								const UINT SourceWidth,
								const UINT SourceHeight,
								const UINT SourceRowPitch,
								const float U,
								const float V,
								float * OutRgb)
	{
		const float X = U * static_cast<float>(SourceWidth) - 0.5f;
		const float Y = V * static_cast<float>(SourceHeight) - 0.5f;

		const int X0 = static_cast<int>(floorf(X));
		const int Y0 = static_cast<int>(floorf(Y));
		const float Fx = X - static_cast<float>(X0);
		const float Fy = Y - static_cast<float>(Y0);

		const auto WrapX = [SourceWidth](int Value) -> UINT
		{
			const int W = static_cast<int>(SourceWidth);
			Value %= W;
			return static_cast<UINT>(Value < 0 ? Value + W : Value);
		};
		const auto ClampY = [SourceHeight](int Value) -> UINT
		{
			const int H = static_cast<int>(SourceHeight);
			return static_cast<UINT>(Value < 0 ? 0 : (Value >= H ? H - 1 : Value));
		};

		const UINT Xs[2] = { WrapX(X0),  WrapX(X0 + 1) };
		const UINT Ys[2] = { ClampY(Y0), ClampY(Y0 + 1) };

		for (UINT Channel = 0; Channel < 3; ++Channel)
		{
			const float C00 = Source[Ys[0] * SourceRowPitch + Xs[0] * BYTES_PER_TEXEL + Channel];
			const float C10 = Source[Ys[0] * SourceRowPitch + Xs[1] * BYTES_PER_TEXEL + Channel];
			const float C01 = Source[Ys[1] * SourceRowPitch + Xs[0] * BYTES_PER_TEXEL + Channel];
			const float C11 = Source[Ys[1] * SourceRowPitch + Xs[1] * BYTES_PER_TEXEL + Channel];

			const float Top		= C00 + (C10 - C00) * Fx;
			const float Bottom	= C01 + (C11 - C01) * Fx;

			OutRgb[Channel] = Top + (Bottom - Top) * Fy;
		}
	}

	// Box filters one mip down into the next, per face
	std::vector<BYTE> DownsampleFaces(const std::vector<BYTE>& Source, const UINT SourceResolution)
	{
		const UINT DestResolution	= SourceResolution / 2;
		const SIZE_T SourcePitch	= SIZE_T(SourceResolution) * BYTES_PER_TEXEL;
		const SIZE_T SourceFace		= SourcePitch * SourceResolution;
		const SIZE_T DestPitch		= SIZE_T(DestResolution) * BYTES_PER_TEXEL;
		const SIZE_T DestFace		= DestPitch * DestResolution;

		std::vector<BYTE> Dest(DestFace * CubeMap::FACE_COUNT);

		for (UINT FaceIdx = 0; FaceIdx < CubeMap::FACE_COUNT; ++FaceIdx)
		{
			const BYTE * const Src = Source.data() + SourceFace * FaceIdx;
			BYTE * const Dst = Dest.data() + DestFace * FaceIdx;

			for (UINT Y = 0; Y < DestResolution; ++Y)
			{
				for (UINT X = 0; X < DestResolution; ++X)
				{
					for (UINT Channel = 0; Channel < BYTES_PER_TEXEL; ++Channel)
					{
						const UINT Sum =
							Src[(SIZE_T(Y * 2 + 0) * SourcePitch) + (SIZE_T(X * 2 + 0) * BYTES_PER_TEXEL) + Channel] +
							Src[(SIZE_T(Y * 2 + 0) * SourcePitch) + (SIZE_T(X * 2 + 1) * BYTES_PER_TEXEL) + Channel] +
							Src[(SIZE_T(Y * 2 + 1) * SourcePitch) + (SIZE_T(X * 2 + 0) * BYTES_PER_TEXEL) + Channel] +
							Src[(SIZE_T(Y * 2 + 1) * SourcePitch) + (SIZE_T(X * 2 + 1) * BYTES_PER_TEXEL) + Channel];

						Dst[(SIZE_T(Y) * DestPitch) + (SIZE_T(X) * BYTES_PER_TEXEL) + Channel] = static_cast<BYTE>(Sum / 4);
					}
				}
			}
		}

		return Dest;
	}

	BYTE ToByte(const float Value)
	{
		const float Clamped = Value < 0.f ? 0.f : (Value > 255.f ? 255.f : Value);
		return static_cast<BYTE>(Clamped + 0.5f);
	}
}

void CubeMap::LoadRadiance(DXDevice * Device,
							DXGraphicsCommandList * CommandList,
							const std::wstring& PanoramaPath,
							const UINT FaceResolution,
							const std::wstring& Name)
{
	DXASSERT(Device && CommandList, "Device and command list can not be null for loading a cube map");
	DXASSERT(!IsValid(), "Cube map has been already loaded");
	DXASSERT(FaceResolution > 0, "Cube map needs a non empty face resolution");

	this->FaceResolution = FaceResolution;

	// Decode the panorama off disk
	D3D12_RESOURCE_DESC PanoramaDesc;
	int PanoramaBytesPerRow = 0;
	SimpleSmartPointer<BYTE> PanoramaData = nullptr;
	LoadImageDataFromFile(PanoramaData, PanoramaDesc, PanoramaPath.c_str(), PanoramaBytesPerRow);

	DXASSERT(PanoramaData.Get(), "Panorama could not be decoded, check the file next to the executable");
	DXASSERT(PanoramaDesc.Format == GetFormat(), "Panorama decoded into an unexpected format, the face projection assumes 8 bits per channel RGBA");

	const auto SourceWidth	= static_cast<UINT>(PanoramaDesc.Width);
	const auto SourceHeight	= static_cast<UINT>(PanoramaDesc.Height);

	// Project every face out of the panorama. For each texel the direction it looks at is
	// turned into a latitude and longitude, which is where the panorama stores that pixel.
	const SIZE_T FaceRowPitch	= SIZE_T(FaceResolution) * BYTES_PER_TEXEL;
	const SIZE_T FaceSize		= FaceRowPitch * FaceResolution;

	std::vector<FaceData> Mips;
	Mips.emplace_back(FaceSize * FACE_COUNT);

	for (UINT FaceIdx = 0; FaceIdx < FACE_COUNT; ++FaceIdx)
	{
		BYTE * const Face = Mips[0].data() + (FaceSize * FaceIdx);

		for (UINT Y = 0; Y < FaceResolution; ++Y)
		{
			// Texel centres, and Vc flipped because the face V axis grows downwards
			const float Vc = 1.f - 2.f * ((static_cast<float>(Y) + 0.5f) / static_cast<float>(FaceResolution));

			for (UINT X = 0; X < FaceResolution; ++X)
			{
				const float Uc = 2.f * ((static_cast<float>(X) + 0.5f) / static_cast<float>(FaceResolution)) - 1.f;

				const auto Direction = XMVector3Normalize(GetFaceDirection(FaceIdx, Uc, Vc));

				float U = 0.f;
				float V = 0.f;
				DirectionToEquirectangular(Direction, U, V);

				float Rgb[3] = {};
				SampleEquirectangular(PanoramaData.Get(), SourceWidth, SourceHeight, static_cast<UINT>(PanoramaBytesPerRow), U, V, Rgb);

				BYTE * const Texel = Face + (SIZE_T(Y) * FaceRowPitch) + (SIZE_T(X) * BYTES_PER_TEXEL);
				Texel[0] = ToByte(Rgb[0]);
				Texel[1] = ToByte(Rgb[1]);
				Texel[2] = ToByte(Rgb[2]);
				Texel[3] = 255;
			}
		}
	}

	// Every smaller mip is a blurrier sky, which is what a rough surface reflects
	UINT MipResolution = FaceResolution;
	while (MipResolution > 1)
	{
		Mips.emplace_back(DownsampleFaces(Mips.back(), MipResolution));
		MipResolution /= 2;
	}

	MipCount = static_cast<UINT>(Mips.size());
	Upload(Device, CommandList, Mips, Name);
}

void CubeMap::LoadIrradiance(DXDevice * Device,
							DXGraphicsCommandList * CommandList,
							const std::wstring& PanoramaPath,
							const UINT FaceResolution,
							const std::wstring& Name)
{
	DXASSERT(Device && CommandList, "Device and command list can not be null for loading a cube map");
	DXASSERT(!IsValid(), "Cube map has been already loaded");
	DXASSERT(FaceResolution > 0, "Cube map needs a non empty face resolution");

	this->FaceResolution	= FaceResolution;
	this->MipCount			= 1;	// irradiance is already as smooth as it gets

	D3D12_RESOURCE_DESC PanoramaDesc;
	int PanoramaBytesPerRow = 0;
	SimpleSmartPointer<BYTE> PanoramaData = nullptr;
	LoadImageDataFromFile(PanoramaData, PanoramaDesc, PanoramaPath.c_str(), PanoramaBytesPerRow);

	DXASSERT(PanoramaData.Get(), "Panorama could not be decoded, check the file next to the executable");

	const auto SourceWidth	= static_cast<UINT>(PanoramaDesc.Width);
	const auto SourceHeight	= static_cast<UINT>(PanoramaDesc.Height);

	const SIZE_T FaceRowPitch	= SIZE_T(FaceResolution) * BYTES_PER_TEXEL;
	const SIZE_T FaceSize		= FaceRowPitch * FaceResolution;

	std::vector<FaceData> Mips;
	Mips.emplace_back(FaceSize * FACE_COUNT);

	for (UINT FaceIdx = 0; FaceIdx < FACE_COUNT; ++FaceIdx)
	{
		BYTE * const Face = Mips[0].data() + (FaceSize * FaceIdx);

		for (UINT Y = 0; Y < FaceResolution; ++Y)
		{
			const float Vc = 1.f - 2.f * ((static_cast<float>(Y) + 0.5f) / static_cast<float>(FaceResolution));

			for (UINT X = 0; X < FaceResolution; ++X)
			{
				const float Uc = 2.f * ((static_cast<float>(X) + 0.5f) / static_cast<float>(FaceResolution)) - 1.f;

				// Normal this texel stands for
				const auto Normal = XMVector3Normalize(GetFaceDirection(FaceIdx, Uc, Vc));

				// Tangent basis around it, so the hemisphere can be walked in polar angles
				const auto Reference	= fabsf(XMVectorGetY(Normal)) > 0.99f
											? XMVectorSet(0.f, 0.f, 1.f, 0.f)
											: XMVectorSet(0.f, 1.f, 0.f, 0.f);
				const auto Tangent		= XMVector3Normalize(XMVector3Cross(Reference, Normal));
				const auto Bitangent	= XMVector3Cross(Normal, Tangent);

				// Cosine weighted integral of the sky over the hemisphere
				float Sum[3]		= {};
				float WeightSum		= 0.f;

				for (UINT PhiIdx = 0; PhiIdx < IRRADIANCE_PHI_SAMPLES; ++PhiIdx)
				{
					const float Phi = XM_2PI * (static_cast<float>(PhiIdx) + 0.5f) / static_cast<float>(IRRADIANCE_PHI_SAMPLES);

					for (UINT ThetaIdx = 0; ThetaIdx < IRRADIANCE_THETA_SAMPLES; ++ThetaIdx)
					{
						// Theta walks from the normal out to the horizon
						const float Theta	= XM_PIDIV2 * (static_cast<float>(ThetaIdx) + 0.5f) / static_cast<float>(IRRADIANCE_THETA_SAMPLES);
						const float SinT	= sinf(Theta);
						const float CosT	= cosf(Theta);

						// Hemisphere direction brought into world space
						const auto SampleDir = XMVector3Normalize(
							XMVectorAdd(
								XMVectorAdd(XMVectorScale(Tangent,   SinT * cosf(Phi)),
											XMVectorScale(Bitangent, SinT * sinf(Phi))),
								XMVectorScale(Normal, CosT)));

						float U = 0.f;
						float V = 0.f;
						DirectionToEquirectangular(SampleDir, U, V);

						float Rgb[3] = {};
						SampleEquirectangular(PanoramaData.Get(), SourceWidth, SourceHeight, static_cast<UINT>(PanoramaBytesPerRow), U, V, Rgb);

						// Lambert weighting, plus the sin that comes with polar integration
						const float Weight = CosT * SinT;
						Sum[0] += Rgb[0] * Weight;
						Sum[1] += Rgb[1] * Weight;
						Sum[2] += Rgb[2] * Weight;
						WeightSum += Weight;
					}
				}

				BYTE * const Texel = Face + (SIZE_T(Y) * FaceRowPitch) + (SIZE_T(X) * BYTES_PER_TEXEL);
				const float InvWeight = WeightSum > 0.f ? 1.f / WeightSum : 0.f;
				Texel[0] = ToByte(Sum[0] * InvWeight);
				Texel[1] = ToByte(Sum[1] * InvWeight);
				Texel[2] = ToByte(Sum[2] * InvWeight);
				Texel[3] = 255;
			}
		}
	}

	Upload(Device, CommandList, Mips, Name);
}

void CubeMap::Upload(DXDevice * Device,
					DXGraphicsCommandList * CommandList,
					const std::vector<FaceData>& Mips,
					const std::wstring& Name)
{
	// A cube map is a six slice 2D array, the cube part is how the view reads it
	const auto CubeDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		GetFormat(),
		FaceResolution,
		FaceResolution,
		FACE_COUNT,							// array size, one slice per face
		static_cast<UINT16>(MipCount));

	constexpr auto StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
	constexpr auto StateAfter	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT,	// a default heap
		D3D12_HEAP_FLAG_NONE,		// no flags
		&CubeDesc,					// the description of the cube
		StateBefore,				// copied into first, then sampled
		nullptr,					// used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&CubeBuffer)));
	CubeBuffer->SetName(Name.c_str());

	const UINT SubresourceCount = FACE_COUNT * MipCount;

	// Upload heap big enough for every face of every mip at once
	const UINT64 UploadSize = GetRequiredIntermediateSize(CubeBuffer.Get(), 0, SubresourceCount);
	const auto UploadDesc	= CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_UPLOAD,
		D3D12_HEAP_FLAG_NONE,
		&UploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&CubeBufferUploadHeap)));
	CubeBufferUploadHeap->SetName((Name + L" Upload Resource Heap").c_str());

	// Subresources run mip major inside each array slice, so the index of face F mip M
	// is M + F * MipCount
	std::vector<D3D12_SUBRESOURCE_DATA> Subresources(SubresourceCount);

	for (UINT FaceIdx = 0; FaceIdx < FACE_COUNT; ++FaceIdx)
	{
		UINT MipResolution = FaceResolution;

		for (UINT MipIdx = 0; MipIdx < MipCount; ++MipIdx)
		{
			const SIZE_T MipRowPitch	= SIZE_T(MipResolution) * BYTES_PER_TEXEL;
			const SIZE_T MipFaceSize	= MipRowPitch * MipResolution;

			auto& Subresource		= Subresources[MipIdx + FaceIdx * MipCount];
			Subresource.pData		= Mips[MipIdx].data() + (MipFaceSize * FaceIdx);
			Subresource.RowPitch	= MipRowPitch;
			Subresource.SlicePitch	= MipFaceSize;

			MipResolution = MipResolution > 1 ? MipResolution / 2 : 1;
		}
	}

	UpdateSubresources(CommandList, CubeBuffer.Get(), CubeBufferUploadHeap.Get(), 0, 0, SubresourceCount, Subresources.data());

	const auto Transition = CD3DX12_RESOURCE_BARRIER::Transition(CubeBuffer.Get(), StateBefore, StateAfter);
	CommandList->ResourceBarrier(1, &Transition);
}

void CubeMap::CreateShaderResourceView(DXDevice * Device, const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle) const
{
	DXASSERT(Device, "Device can not be null for creating the cube map view");
	DXASSERT(IsValid(), "Cube map must be loaded before creating its view");

	// TextureCube is what makes the shader index it with a direction instead of a uv
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format						= GetFormat();
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURECUBE;
	SrvDesc.TextureCube.MipLevels		= MipCount;
	SrvDesc.TextureCube.MostDetailedMip	= 0;
	Device->CreateShaderResourceView(CubeBuffer.Get(), &SrvDesc, SrvHandle);
}
