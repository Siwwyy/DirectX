#include "D3D12Texture.h"


// Texture class
SimpleSmartPointer<BYTE>&& Texture::Load(DXDevice * Device, DXGraphicsCommandList * CommandList, std::wstring TextureName)
{
	// Usings
	using Helpers::ThrowIfFailed;

	D3D12_RESOURCE_DESC TextureDesc;
	int ImageBytesPerRow = 0;
	SimpleSmartPointer<BYTE> ImageData = nullptr;
	int ImageSize = LoadImageDataFromFile(ImageData, TextureDesc, L"bryanzar.png", ImageBytesPerRow);

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_DEFAULT, // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&TextureDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&TextureBuffer)));
	TextureBuffer->SetName(TextureName.c_str());

	UINT64 TextureUploadBufferSize = 0;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
	//const UINT64 TextureUploadBufferSizeFormula = (((ImageBytesPerRow + 255) & ~255) * (TextureDesc.Height - 1)) + ImageBytesPerRow;
	Device->GetCopyableFootprints(&TextureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &TextureUploadBufferSize);

	// now we create an upload heap to upload our texture to the GPU
	const auto TextureBufferUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(TextureUploadBufferSize);
	ThrowIfFailed(Device->CreateCommittedResource(
		&DX_HEAP_PROPERTY_UPLOAD, // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&TextureBufferUploadDesc, // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&TextureBufferUploadHeap)));
	TextureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA TextureData = {};
	TextureData.pData = static_cast<void*>(ImageData.Get()); // pointer to our image data
	TextureData.RowPitch = ImageBytesPerRow; // size of all our triangle vertex data
	TextureData.SlicePitch = ImageBytesPerRow * TextureDesc.Height; // also the size of our triangle vertex data

	// Now we copy the upload buffer contents to the default heap
	UpdateSubresources(CommandList, TextureBuffer.Get(), TextureBufferUploadHeap.Get(), 0, 0, 1, &TextureData);

	// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
	const auto Transition = CD3DX12_RESOURCE_BARRIER::Transition(TextureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->ResourceBarrier(1, &Transition);

	// create the descriptor heap that will store our srv
	constexpr auto DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.NumDescriptors = 1;
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HeapDesc.Type = DescriptorHeapType;
	ThrowIfFailed(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&MainDescriptorHeap)));

	// now we create a shader resource view (descriptor that points to the texture and describes it)
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format = TextureDesc.Format;
	SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(TextureBuffer.Get(), &SrvDesc, MainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	// Set Increment Descriptor size for TODO possible future multiple textures
	IncrementDescriptorSize = Device->GetDescriptorHandleIncrementSize(DescriptorHeapType);


	// return | Temporary solution!!
	return std::move(ImageData);
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetTextureHandle(SIZE_T TextureIdx)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(MainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	// ptr = UINT64(INT64(ptr) + INT64(offsetInDescriptors) * INT64(descriptorIncrementSize));
	return Handle.Offset(TextureIdx, IncrementDescriptorSize);
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


