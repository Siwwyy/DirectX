#include "D3D12Texture.h"


UINT GetMipCount(const UINT Width, const UINT Height)
{
	UINT Count		= 1;
	UINT MipWidth	= Width;
	UINT MipHeight	= Height;

	while (MipWidth > 1 || MipHeight > 1)
	{
		MipWidth	= MipWidth > 1 ? MipWidth / 2 : 1;
		MipHeight	= MipHeight > 1 ? MipHeight / 2 : 1;
		++Count;
	}

	return Count;
}

namespace
{
	// Width of a single channel in bytes, 0 when the format is not one this can filter.
	// Averaging has to happen per channel, not per byte: halving a 16 bit value by
	// averaging its two bytes separately produces noise, not a smaller image.
	UINT GetFormatChannelBytes(const DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_A8_UNORM:
			return 1;

		// Poly Haven ships its roughness, metalness and occlusion maps as 16 bit
		// grayscale, so this one matters as much as the 8 bit case
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return 2;

		default:
			return 0;
		}
	}
}

bool CanGenerateMips(const DXGI_FORMAT Format)
{
	return GetFormatChannelBytes(Format) != 0;
}

UINT GetFormatBytesPerTexel(const DXGI_FORMAT Format)
{
	// The bits per pixel helper takes its argument by non const reference and answers -1
	// for anything it does not recognise, so both have to be worked around here
	auto LocalFormat		= Format;
	const int BitsPerPixel	= GetDXGIFormatBitsPerPixel(LocalFormat);

	return BitsPerPixel > 0 ? static_cast<UINT>(BitsPerPixel) / 8 : 0;
}

std::vector<std::vector<BYTE>> GenerateMipChain(const BYTE * Source, const UINT Width, const UINT Height, const UINT RowPitch, const DXGI_FORMAT Format)
{
	DXASSERT(Source, "Mip generation needs an image to start from");
	DXASSERT(Width > 0 && Height > 0, "Mip generation needs a non empty image");

	// How wide one texel is, straight out of the format. This is the thing that broke
	// when it was assumed to be four: a grayscale map is one byte and reading it four
	// bytes at a time runs clean off the end of the decoded image.
	const UINT BytesPerTexel = GetFormatBytesPerTexel(Format);

	DXASSERT(BytesPerTexel > 0, "Unknown format, the texel size can not be worked out");
	DXASSERT(RowPitch >= Width * BytesPerTexel, "Row pitch is smaller than one row of texels, reading it would run off the end");

	const UINT ChannelBytes	= GetFormatChannelBytes(Format);
	const bool bCanFilter	= ChannelBytes != 0;

	std::vector<std::vector<BYTE>> Mips;

	// Mip 0 is the image itself, copied so every mip is laid out the same way
	{
		const SIZE_T Pitch = SIZE_T(Width) * BytesPerTexel;
		std::vector<BYTE> Base(Pitch * Height);

		for (UINT Y = 0; Y < Height; ++Y)
		{
			memcpy(Base.data() + (SIZE_T(Y) * Pitch), Source + (SIZE_T(Y) * RowPitch), Pitch);
		}

		Mips.emplace_back(std::move(Base));
	}

	// A format whose channels are wider than a byte can not be averaged this way, so it
	// stops here with just the original
	if (!bCanFilter)
	{
		return Mips;
	}

	UINT MipWidth	= Width;
	UINT MipHeight	= Height;

	while (MipWidth > 1 || MipHeight > 1)
	{
		const UINT SrcWidth		= MipWidth;
		const UINT SrcHeight	= MipHeight;

		// A dimension that already bottomed out stays at 1, which is what keeps a
		// non square image going until both axes reach the end
		MipWidth	= SrcWidth > 1 ? SrcWidth / 2 : 1;
		MipHeight	= SrcHeight > 1 ? SrcHeight / 2 : 1;

		const SIZE_T SrcPitch	= SIZE_T(SrcWidth) * BytesPerTexel;
		const SIZE_T DstPitch	= SIZE_T(MipWidth) * BytesPerTexel;

		const auto& Src = Mips.back();
		std::vector<BYTE> Dst(DstPitch * MipHeight);

		for (UINT Y = 0; Y < MipHeight; ++Y)
		{
			// Clamp so a 1 pixel tall mip averages the same row twice rather than
			// reading past the end
			const UINT Y0 = SrcHeight > 1 ? Y * 2 : 0;
			const UINT Y1 = SrcHeight > 1 ? Y * 2 + 1 : 0;

			for (UINT X = 0; X < MipWidth; ++X)
			{
				const UINT X0 = SrcWidth > 1 ? X * 2 : 0;
				const UINT X1 = SrcWidth > 1 ? X * 2 + 1 : 0;

				// Offsets of the four texels being averaged, and of where the result goes
				const SIZE_T Src00	= (SIZE_T(Y0) * SrcPitch) + (SIZE_T(X0) * BytesPerTexel);
				const SIZE_T Src10	= (SIZE_T(Y0) * SrcPitch) + (SIZE_T(X1) * BytesPerTexel);
				const SIZE_T Src01	= (SIZE_T(Y1) * SrcPitch) + (SIZE_T(X0) * BytesPerTexel);
				const SIZE_T Src11	= (SIZE_T(Y1) * SrcPitch) + (SIZE_T(X1) * BytesPerTexel);
				const SIZE_T DstAt	= (SIZE_T(Y) * DstPitch) + (SIZE_T(X) * BytesPerTexel);

				const UINT ChannelCount = BytesPerTexel / ChannelBytes;

				for (UINT Channel = 0; Channel < ChannelCount; ++Channel)
				{
					const SIZE_T ChannelOffset = SIZE_T(Channel) * ChannelBytes;

					if (ChannelBytes == 1)
					{
						const UINT Sum =
							Src[Src00 + ChannelOffset] + Src[Src10 + ChannelOffset] +
							Src[Src01 + ChannelOffset] + Src[Src11 + ChannelOffset];

						Dst[DstAt + ChannelOffset] = static_cast<BYTE>(Sum / 4);
					}
					else
					{
						// Read the pair of bytes as the value it actually is before averaging
						const auto Read = [&](const SIZE_T At) -> UINT
						{
							UINT16 Value = 0;
							memcpy(&Value, &Src[At + ChannelOffset], sizeof(Value));
							return Value;
						};

						const UINT Sum = Read(Src00) + Read(Src10) + Read(Src01) + Read(Src11);

						const UINT16 Average = static_cast<UINT16>(Sum / 4);
						memcpy(&Dst[DstAt + ChannelOffset], &Average, sizeof(Average));
					}
				}
			}
		}

		Mips.emplace_back(std::move(Dst));
	}

	return Mips;
}


// Functions
int LoadImageDataFromFile(SimpleSmartPointer<BYTE>& ImageData, D3D12_RESOURCE_DESC& ResourceDescription, LPCWSTR Filename, int& BytesPerRow)
{
	// we only need one instance of the imaging factory to create decoders and frames
	static IWICImagingFactory* WicFactory = nullptr;

	// reset decoder, frame and converter since these will be different for each image we load
	IWICBitmapDecoder* WicDecoder = nullptr;
	IWICBitmapFrameDecode* WicFrame = nullptr;
	IWICFormatConverter* WicConverter = nullptr;

	bool ImageConverted = false;

	if (WicFactory == nullptr)
	{
		// Initialize the COM library
		CoInitialize(nullptr);

		// create the WIC factory
		Helpers::ThrowIfFailed(CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&WicFactory)));
	}

	// load a decoder for the image
	Helpers::ThrowIfFailed(WicFactory->CreateDecoderFromFilename(
		Filename,                        // Image we want to load in
		NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
		GENERIC_READ,                    // We want to read from this file
		WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
		&WicDecoder                      // the wic decoder to be created
	));

	// get image from decoder (this will decode the "frame")
	Helpers::ThrowIfFailed(WicDecoder->GetFrame(0, &WicFrame));

	// get wic pixel format of image
	WICPixelFormatGUID pixelFormat;
	Helpers::ThrowIfFailed(WicFrame->GetPixelFormat(&pixelFormat));

	// get size of image
	UINT TextureWidth = 0;
	UINT TextureHeight = 0;
	Helpers::ThrowIfFailed(WicFrame->GetSize(&TextureWidth, &TextureHeight));

	// we are not handling sRGB types in this tutorial, so if you need that support, you'll have to figure
	// out how to implement the support yourself

	// convert wic pixel format to dxgi pixel format
	DXGI_FORMAT DxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

	// if the format of the image is not a supported dxgi format, try to convert it
	if (DxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		// get a dxgi compatible wic format from the current image format
		WICPixelFormatGUID ConvertToPixelFormat = GetConvertToWICFormat(pixelFormat);

		// return if no dxgi compatible format was found
		if (ConvertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

		// set the dxgi format
		DxgiFormat = GetDXGIFormatFromWICFormat(ConvertToPixelFormat);

		// create the format converter
		Helpers::ThrowIfFailed(WicFactory->CreateFormatConverter(&WicConverter));

		// make sure we can convert to the dxgi compatible format
		BOOL canConvert = FALSE;
		Helpers::ThrowIfFailed(WicConverter->CanConvert(pixelFormat, ConvertToPixelFormat, &canConvert));

		// do the conversion (WicConverter will contain the converted image)
		Helpers::ThrowIfFailed(WicConverter->Initialize(WicFrame, ConvertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom));

		// this is so we know to get the image data from the WicConverter (otherwise we will get from WicFrame)
		ImageConverted = true;
	}

	int BitsPerPixel = GetDXGIFormatBitsPerPixel(DxgiFormat); // number of bits per pixel
	BytesPerRow = (TextureWidth * BitsPerPixel) / 8; // number of bytes in each row of the image data
	int ImageSize = BytesPerRow * TextureHeight; // total image size in bytes

	// allocate enough memory for the raw image data, and set imageData to point to that memory
	ImageData = make_simple_smart_pointer<BYTE>(ImageSize);

	// copy (decoded) raw image data into the newly allocated memory (imageData)
	if (ImageConverted)
	{
		// if image format needed to be converted, the wic converter will contain the converted image
		Helpers::ThrowIfFailed(WicConverter->CopyPixels(0, BytesPerRow, ImageSize, ImageData.Get()));
	}
	else
	{
		// no need to convert, just copy data from the wic frame
		Helpers::ThrowIfFailed(WicFrame->CopyPixels(0, BytesPerRow, ImageSize, ImageData.Get()));
	}

	// now describe the texture with the information we have obtained from the image
	ResourceDescription = {};
	ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	ResourceDescription.Width = TextureWidth; // width of the texture
	ResourceDescription.Height = TextureHeight; // height of the texture
	ResourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	ResourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	ResourceDescription.Format = DxgiFormat; // This is the dxgi format of the image (format of the pixels)
	ResourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	ResourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	ResourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	ResourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	// return the size of the image. remember to delete the image once your done with it (in this tutorial once its uploaded to the gpu)
	return ImageSize;
}

DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& WicFormatGUID)
{
	if (WicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (WicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (WicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (WicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;
	else return DXGI_FORMAT_UNKNOWN;
}

WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& WicFormatGUID)
{
	if (WicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (WicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (WicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (WicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (WicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (WicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (WicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (WicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (WicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (WicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

	else return GUID_WICPixelFormatDontCare;
}

int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& DxgiFormat)
{
	if (DxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (DxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (DxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (DxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (DxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (DxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (DxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;
	else if (DxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (DxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (DxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (DxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (DxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (DxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (DxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (DxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	return -1;
}