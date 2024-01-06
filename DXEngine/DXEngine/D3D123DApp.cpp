#include "D3D123DApp.h"

#include "Win32Proc.h"
#include "D3D12Math.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Utils.h"

#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <dxcapi.h>



//CONSTANTS
_CONSTEVAL D3D12_COMMAND_LIST_TYPE		COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;
_CONSTEVAL UINT							BACK_BUFFER_COUNT = 2;
_CONSTEVAL D3D_FEATURE_LEVEL			D3D12_FEATURE_LEVEL = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
_CONSTEVAL DXGI_FORMAT					BACK_BUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
_CONSTEVAL DXGI_FORMAT					DEPTH_STENCIL_FORMAT = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;		//depth stencil format


struct alignas(256) SceneConstantBuffer
{
	DirectX::XMFLOAT4X4 wvpMat;
};
static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

using namespace Helpers;
using namespace Math;
using namespace Utils;


D3D123DApp::D3D123DApp(const UINT windowWidth, const UINT windowHeight, const std::wstring windowName)
	: windowWidth(windowWidth)
	, windowHeight(windowHeight)
	, aspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight))
	, windowName(windowName)
	, viewPort(0.f, 0.f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 1.0f)
	, scissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight))
	, bufferCount(BACK_BUFFER_COUNT)
	, currentFrameIdx(0)
	, rtvIncrementDescriptorSize(0)
{
	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");

	// Render Targets
	renderTargets.resize(BACK_BUFFER_COUNT);
}

void D3D123DApp::Initialize()
{
	// Initialize factory
	UINT dxgiFactoryFlags = 0;

#if DEBUG_MODE
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	////

	// Initialize device
	const ComPtr<IDXGIAdapter1> hardwareAdapter = GetAdapter(factory, D3D12_FEATURE_LEVEL);
	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D12_FEATURE_LEVEL,
		IID_PPV_ARGS(&device)
	));

	////



}

void D3D123DApp::Render()
{

}

void D3D123DApp::Update()
{

}

void D3D123DApp::Destroy()
{

}

void D3D123DApp::PopulateCommandLists()
{

}

void D3D123DApp::WaitForPreviousFrame()
{
}
