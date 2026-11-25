//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "ExrWriter.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <numeric>

namespace
{
	// ---------------------------------------------------------------------------------
	//  The parts of the format this file uses. All of it little endian, which is also what
	//  every machine this builds for is, so values are copied out as they sit in memory.
	//
	//      magic            4 bytes   0x76 0x2f 0x31 0x01
	//      version          4 bytes   2, then three bytes of flags that are all zero
	//                                 (single part, scanlines, short names)
	//      header           attributes, each: name\0, type\0, int32 size, size bytes
	//                       ended by a single 0 byte
	//      offset table     one uint64 per scanline block: where that block starts in the file
	//      scanline blocks  each: int32 y, int32 byte count, then the pixels
	//
	//  With no compression a block is a single scanline, and holds each channel's pixels for
	//  that row one after the other, channels in the header's order.
	// ---------------------------------------------------------------------------------

	constexpr uint8_t	MAGIC[4]			= { 0x76, 0x2f, 0x31, 0x01 };
	constexpr uint8_t	VERSION[4]			= { 2, 0, 0, 0 };

	constexpr int32_t	PIXEL_TYPE_FLOAT	= 2;	// 32 bit float, the others are uint (0) and half (1)
	constexpr uint8_t	COMPRESSION_NONE	= 0;
	constexpr uint8_t	LINE_ORDER_INCREASING_Y = 0;

	// Growing byte buffer the header is assembled in, so its size is known before it is written
	struct ByteWriter
	{
		std::vector<uint8_t> Bytes;

		void Raw(const void * const Data, const size_t Size)
		{
			const auto * const Begin = static_cast<const uint8_t*>(Data);
			Bytes.insert(Bytes.end(), Begin, Begin + Size);
		}

		void Int32(const int32_t Value)		{ Raw(&Value, sizeof(Value)); }
		void Float32(const float Value)		{ Raw(&Value, sizeof(Value)); }
		void Byte(const uint8_t Value)		{ Bytes.push_back(Value); }

		// Names and types are stored as text with a terminating zero
		void String(const std::string& Text)
		{
			Raw(Text.c_str(), Text.size() + 1);
		}

		// An attribute is its name, its type, the size of its payload and the payload
		void Attribute(const std::string& Name, const std::string& Type, const ByteWriter& Payload)
		{
			String(Name);
			String(Type);
			Int32(static_cast<int32_t>(Payload.Bytes.size()));
			Raw(Payload.Bytes.data(), Payload.Bytes.size());
		}
	};

	// The attributes every EXR must carry, in the order the reference writer puts them
	void WriteHeader(ByteWriter& Header,
					 const std::vector<const DXIEXR::Channel*>& SortedChannels,
					 const uint32_t Width,
					 const uint32_t Height)
	{
		// Channel list: per channel its name, pixel type, a linear flag, three padding bytes
		// and its sampling, ended by an empty name. Sampling 1 and 1 means a value per pixel.
		ByteWriter ChannelList;
		for (const auto * const Channel : SortedChannels)
		{
			ChannelList.String(Channel->Name);
			ChannelList.Int32(PIXEL_TYPE_FLOAT);
			ChannelList.Byte(0);				// pLinear, only a hint for lossy compression
			ChannelList.Byte(0);				// reserved
			ChannelList.Byte(0);
			ChannelList.Byte(0);
			ChannelList.Int32(1);				// xSampling
			ChannelList.Int32(1);				// ySampling
		}
		ChannelList.Byte(0);
		Header.Attribute("channels", "chlist", ChannelList);

		ByteWriter Compression;
		Compression.Byte(COMPRESSION_NONE);
		Header.Attribute("compression", "compression", Compression);

		// Both windows are the whole image. The data window is inclusive, so it ends at size - 1
		ByteWriter Window;
		Window.Int32(0);
		Window.Int32(0);
		Window.Int32(static_cast<int32_t>(Width) - 1);
		Window.Int32(static_cast<int32_t>(Height) - 1);
		Header.Attribute("dataWindow", "box2i", Window);
		Header.Attribute("displayWindow", "box2i", Window);

		ByteWriter LineOrder;
		LineOrder.Byte(LINE_ORDER_INCREASING_Y);
		Header.Attribute("lineOrder", "lineOrder", LineOrder);

		ByteWriter PixelAspectRatio;
		PixelAspectRatio.Float32(1.0f);
		Header.Attribute("pixelAspectRatio", "float", PixelAspectRatio);

		ByteWriter ScreenWindowCenter;
		ScreenWindowCenter.Float32(0.0f);
		ScreenWindowCenter.Float32(0.0f);
		Header.Attribute("screenWindowCenter", "v2f", ScreenWindowCenter);

		ByteWriter ScreenWindowWidth;
		ScreenWindowWidth.Float32(1.0f);
		Header.Attribute("screenWindowWidth", "float", ScreenWindowWidth);

		// The header ends with an attribute with no name
		Header.Byte(0);
	}

	bool Fail(std::string * const OutError, const std::string& Reason)
	{
		if (OutError)
		{
			*OutError = Reason;
		}
		return false;
	}
}

namespace DXIEXR
{
	bool WriteFile(const std::wstring& Path,
					const uint32_t Width,
					const uint32_t Height,
					const std::vector<Channel>& Channels,
					std::string * const OutError)
	{
		if (Width == 0 || Height == 0)
		{
			return Fail(OutError, "image has no pixels");
		}
		if (Channels.empty())
		{
			return Fail(OutError, "image has no channels");
		}

		const size_t PixelCount = static_cast<size_t>(Width) * Height;
		for (const auto& Channel : Channels)
		{
			if (Channel.Name.empty())
			{
				return Fail(OutError, "a channel has no name");
			}
			if (Channel.Pixels.size() != PixelCount)
			{
				return Fail(OutError, "channel '" + Channel.Name + "' does not have Width * Height pixels");
			}
		}

		// EXR wants the channels sorted by name, byte order of the names, which is also what
		// readers use to find where each channel sits inside a scanline
		std::vector<const Channel*> Sorted;
		Sorted.reserve(Channels.size());
		for (const auto& Channel : Channels)
		{
			Sorted.push_back(&Channel);
		}
		std::sort(Sorted.begin(), Sorted.end(), [](const Channel* A, const Channel* B) { return A->Name < B->Name; });

		for (size_t Idx = 1; Idx < Sorted.size(); ++Idx)
		{
			if (Sorted[Idx]->Name == Sorted[Idx - 1]->Name)
			{
				return Fail(OutError, "two channels are both named '" + Sorted[Idx]->Name + "'");
			}
		}

		ByteWriter Header;
		WriteHeader(Header, Sorted, Width, Height);

		// Layout of the file, worked out up front because the offset table sits in front of
		// the data it points at
		const uint64_t ScanlineDataBytes	= static_cast<uint64_t>(Width) * sizeof(float) * Sorted.size();
		const uint64_t BlockBytes			= sizeof(int32_t) + sizeof(int32_t) + ScanlineDataBytes;
		const uint64_t OffsetTableBytes		= static_cast<uint64_t>(Height) * sizeof(uint64_t);
		const uint64_t FirstBlockOffset		= sizeof(MAGIC) + sizeof(VERSION) + Header.Bytes.size() + OffsetTableBytes;

		std::ofstream File(Path, std::ios::binary | std::ios::trunc);
		if (!File)
		{
			return Fail(OutError, "could not open the file for writing");
		}

		File.write(reinterpret_cast<const char*>(MAGIC), sizeof(MAGIC));
		File.write(reinterpret_cast<const char*>(VERSION), sizeof(VERSION));
		File.write(reinterpret_cast<const char*>(Header.Bytes.data()), static_cast<std::streamsize>(Header.Bytes.size()));

		// Offset table, one entry per scanline, every block being the same size
		for (uint32_t Y = 0; Y < Height; ++Y)
		{
			const uint64_t Offset = FirstBlockOffset + (static_cast<uint64_t>(Y) * BlockBytes);
			File.write(reinterpret_cast<const char*>(&Offset), sizeof(Offset));
		}

		// Scanline blocks
		for (uint32_t Y = 0; Y < Height; ++Y)
		{
			const int32_t YCoordinate	= static_cast<int32_t>(Y);
			const int32_t DataSize		= static_cast<int32_t>(ScanlineDataBytes);
			File.write(reinterpret_cast<const char*>(&YCoordinate), sizeof(YCoordinate));
			File.write(reinterpret_cast<const char*>(&DataSize), sizeof(DataSize));

			// This row of every channel, in the sorted order the header lists them in
			for (const auto * const Channel : Sorted)
			{
				const float * const Row = Channel->Pixels.data() + (static_cast<size_t>(Y) * Width);
				File.write(reinterpret_cast<const char*>(Row), static_cast<std::streamsize>(Width * sizeof(float)));
			}
		}

		File.flush();
		if (!File)
		{
			return Fail(OutError, "writing the file failed part way, the disk may be full");
		}

		return true;
	}
}
