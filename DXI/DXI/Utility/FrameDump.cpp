//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "FrameDump.h"
#include "ExrWriter.h"

#include <chrono>
#include <ctime>
#include <filesystem>

namespace
{
	using DXIEXR::Channel;

	// One channel of an interleaved pixel array, named for the file
	Channel Extract(const std::vector<float>& Interleaved, const UINT ChannelCount, const UINT Index, const char* Name)
	{
		Channel Out;
		Out.Name = Name;

		const size_t PixelCount = Interleaved.size() / ChannelCount;
		Out.Pixels.resize(PixelCount);
		for (size_t Idx = 0; Idx < PixelCount; ++Idx)
		{
			Out.Pixels[Idx] = Interleaved[(Idx * ChannelCount) + Index];
		}
		return Out;
	}

	// Folder name from the clock, sortable and legal on Windows, 20260101_153045
	std::wstring MakeTimestamp()
	{
		const std::time_t Now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm Local = {};
		localtime_s(&Local, &Now);

		wchar_t Text[32];
		wcsftime(Text, std::size(Text), L"%Y%m%d_%H%M%S", &Local);
		return Text;
	}

	std::wstring Widen(const std::string& Narrow)
	{
		return std::wstring(Narrow.begin(), Narrow.end());	// only ever ASCII, the writer's messages are
	}
}

void FrameDump::Capture(const DXIDevice& Device,
						DXGraphicsCommandList * CommandList,
						const DXISwapChain& SwapChain,
						const UINT BackBufferIdx,
						const D3D12_RESOURCE_STATES BackBufferState,
						const GBuffer& SceneGBuffer,
						const DXIDepthBuffer& DepthBuffer,
						const float ProjA,
						const float ProjB)
{
	DXASSERT(!bPending, "The previous dump has not been written yet");

	DXDevice * const D3DDevice = Device.Get();

	ColorReadback.RecordCopy(D3DDevice, CommandList, SwapChain.GetBackBuffer(BackBufferIdx), DXISwapChain::GetFormat(), BackBufferState);

	// The depth texture is typeless, and the copy reads it as the float it is
	DepthReadback.RecordCopy(D3DDevice, CommandList, DepthBuffer.GetResource(), DXIDepthBuffer::GetSrvFormat(), DepthBuffer.GetCurrentState());

	for (UINT Idx = 0; Idx < GBufferTargetCount; ++Idx)
	{
		const auto Target = static_cast<GBufferTarget>(Idx);
		GBufferReadbacks[Idx].RecordCopy(D3DDevice, CommandList, SceneGBuffer.GetResource(Target), GBuffer::GetFormat(Target), SceneGBuffer.GetCurrentState());
	}

	DepthProjA	= ProjA;
	DepthProjB	= ProjB;
	bPending	= true;
}

FrameDump::Result FrameDump::Write(const std::wstring& RootDir)
{
	Result Out;
	if (!bPending)
	{
		Out.Message = L"Nothing was captured";
		return Out;
	}
	bPending = false;

	// A new folder per dump, so one never overwrites another
	namespace fs = std::filesystem;
	const fs::path Folder = fs::path(RootDir) / MakeTimestamp();
	std::error_code FolderError;
	fs::create_directories(Folder, FolderError);
	Out.Directory = Folder.wstring();
	if (FolderError)
	{
		Out.Message = L"Could not create " + Out.Directory;
		return Out;
	}

	std::wstring Failures;

	// Writes one file and keeps going when it fails, so one bad buffer does not cost the others
	auto WriteFile = [&](const wchar_t* FileName, const UINT Width, const UINT Height, const std::vector<Channel>& Channels)
	{
		std::string Error;
		if (DXIEXR::WriteFile((Folder / FileName).wstring(), Width, Height, Channels, &Error))
		{
			++Out.FileCount;
		}
		else
		{
			Failures += std::wstring(FileName) + L": " + Widen(Error) + L"\n";
		}
	};

	std::vector<float> Pixels;

	// Colour, as the screen got it
	if (ColorReadback.Read(Pixels))
	{
		const UINT N = ColorReadback.GetChannelCount();
		WriteFile(L"color.exr", ColorReadback.GetWidth(), ColorReadback.GetHeight(),
			{ Extract(Pixels, N, 0, "R"), Extract(Pixels, N, 1, "G"), Extract(Pixels, N, 2, "B"), Extract(Pixels, N, 3, "A") });
	}
	else
	{
		Failures += L"color.exr: readback failed\n";
	}

	// Depth, both as the buffer holds it and as a distance. The buffer is not linear, nearly all
	// of its precision sits close to the camera, which is useless to look at. A standard
	// perspective projection gives z_clip = Z*A + B and w = Z, so depth = A + B/Z and so Z = B/(depth - A).
	if (DepthReadback.Read(Pixels))
	{
		Channel Linear;
		Linear.Name = "Z";
		Linear.Pixels.resize(Pixels.size());
		for (size_t Idx = 0; Idx < Pixels.size(); ++Idx)
		{
			const float Denominator = Pixels[Idx] - DepthProjA;
			Linear.Pixels[Idx] = (Denominator != 0.f) ? (DepthProjB / Denominator) : 0.f;
		}

		Channel Device;
		Device.Name		= "ZDevice";
		Device.Pixels	= Pixels;

		WriteFile(L"depth.exr", DepthReadback.GetWidth(), DepthReadback.GetHeight(), { Linear, Device });
	}
	else
	{
		Failures += L"depth.exr: readback failed\n";
	}

	// G-buffer
	for (UINT Idx = 0; Idx < GBufferTargetCount; ++Idx)
	{
		const auto Target	= static_cast<GBufferTarget>(Idx);
		const auto& Source	= GBufferReadbacks[Idx];

		if (!Source.Read(Pixels))
		{
			Failures += std::wstring(ToString(Target)) + L": readback failed\n";
			continue;
		}

		const UINT W = Source.GetWidth();
		const UINT H = Source.GetHeight();
		const UINT N = Source.GetChannelCount();

		switch (Target)
		{
		case GBufferTarget::Albedo:
			WriteFile(L"albedo.exr", W, H, { Extract(Pixels, N, 0, "R"), Extract(Pixels, N, 1, "G"), Extract(Pixels, N, 2, "B"), Extract(Pixels, N, 3, "A") });
			break;

		case GBufferTarget::Normal:
			WriteFile(L"normal.exr", W, H, { Extract(Pixels, N, 0, "R"), Extract(Pixels, N, 1, "G"), Extract(Pixels, N, 2, "B") });
			break;

		case GBufferTarget::WorldPosition:
			WriteFile(L"world_position.exr", W, H, { Extract(Pixels, N, 0, "R"), Extract(Pixels, N, 1, "G"), Extract(Pixels, N, 2, "B"), Extract(Pixels, N, 3, "A") });
			break;

		case GBufferTarget::MaterialParams:
			WriteFile(L"material_params.exr", W, H, { Extract(Pixels, N, 0, "R"), Extract(Pixels, N, 1, "G"), Extract(Pixels, N, 2, "B") });

			// Roughness on its own, since it is the one most often wanted by itself. A lone
			// channel is named Y, which is what image viewers show as a grey image.
			WriteFile(L"roughness.exr", W, H, { Extract(Pixels, N, 1, "Y") });
			break;

		case GBufferTarget::Velocity:
			{
				// The target holds a step in uv, 1 being the whole width or height. A step in
				// pixels is that times the size, so a pixel that moved 5 to the right reads 5.
				// Both axes keep the target's orientation, x to the right and y down the screen.
				Channel X = Extract(Pixels, N, 0, "R");
				Channel Y = Extract(Pixels, N, 1, "G");
				for (float& Value : X.Pixels)
				{
					Value *= static_cast<float>(W);
				}
				for (float& Value : Y.Pixels)
				{
					Value *= static_cast<float>(H);
				}
				WriteFile(L"velocity.exr", W, H, { X, Y });
			}
			break;

		default:
			break;
		}
	}

	// The copies are spent, release the memory behind them
	ColorReadback = DXITextureReadback();
	DepthReadback = DXITextureReadback();
	for (auto& Readback : GBufferReadbacks)
	{
		Readback = DXITextureReadback();
	}

	Out.bSuccess = Failures.empty();
	Out.Message  = Out.bSuccess
		? (L"Wrote " + std::to_wstring(Out.FileCount) + L" EXR files to " + Out.Directory)
		: (L"Wrote " + std::to_wstring(Out.FileCount) + L" files, with failures:\n" + Failures);
	return Out;
}
