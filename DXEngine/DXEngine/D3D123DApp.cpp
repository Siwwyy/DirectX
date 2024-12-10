//#include "D3D123DApp.h"
//
//#include "Win32Proc.h"
//#include "D3D12Math.h"
//#include "D3D12ShaderCompiler.h"
//#include "D3D12Utils.h"
//
//#include <D3Dcompiler.h>
//#include <DirectXMath.h>
//#include <dxcapi.h>
//
//
//
////CONSTANTS
//_CONSTEVAL D3D12_COMMAND_LIST_TYPE		COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT;
//_CONSTEVAL UINT							BACK_BUFFER_COUNT = 2;
//_CONSTEVAL D3D_FEATURE_LEVEL			D3D12_FEATURE_LEVEL = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
//_CONSTEVAL DXGI_FORMAT					BACK_BUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
//_CONSTEVAL DXGI_FORMAT					DEPTH_STENCIL_FORMAT = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;		//depth stencil format
//
//
//
//
//using namespace Helpers;
//using namespace Math;
//using namespace Utils;
//
//
//D3D123DApp::D3D123DApp(const UINT windowWidth, const UINT windowHeight, const std::wstring windowName)
//	: windowWidth(windowWidth)
//	, windowHeight(windowHeight)
//	, aspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight))
//	, windowName(windowName)
//	, viewPort(0.f, 0.f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 1.0f)
//	, scissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight))
//	, bufferCount(BACK_BUFFER_COUNT)
//	, currentFrameIdx(0)
//	, rtvIncrementDescriptorSize(0)
//{
//	static_assert(BACK_BUFFER_COUNT > 0, "Back buffer count must be greater than 0!");
//
//	// Render Targets
//	renderTargets.resize(BACK_BUFFER_COUNT);
//}
//
//void D3D123DApp::Initialize()
//{
//	// Initialize factory
//	UINT dxgiFactoryFlags = 0;
//
//#if DEBUG_MODE
//	// Enable the debug layer (requires the Graphics Tools "optional feature").
//	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
//	{
//		ComPtr<ID3D12Debug> debugController;
//		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
//		{
//			debugController->EnableDebugLayer();
//
//			// Enable additional debug layers.
//			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
//		}
//	}
//#endif
//
//	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
//
//	////
//
//	// Initialize device
//	const ComPtr<IDXGIAdapter1> hardwareAdapter = GetAdapter(factory, D3D12_FEATURE_LEVEL);
//	ThrowIfFailed(D3D12CreateDevice(
//		hardwareAdapter.Get(),
//		D3D12_FEATURE_LEVEL,
//		IID_PPV_ARGS(&device)
//	));
//
//	////
//	// 
//	// create root signature
//
//	// create a root descriptor, which explains where to find the data for this root parameter
//	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
//	rootCBVDescriptor.RegisterSpace		= 0;
//	rootCBVDescriptor.ShaderRegister	= 0;
//
//	// create a root parameter and fill it out
//	D3D12_ROOT_PARAMETER rootParameters[1]; // only one parameter right now
//	rootParameters[0].ParameterType		= D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
//	rootParameters[0].Descriptor		= rootCBVDescriptor; // this is the root descriptor for this root parameter
//	rootParameters[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now
//
//	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
//	rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
//		rootParameters, // a pointer to the beginning of our root parameters array
//		0,
//		nullptr,
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
//		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
//		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
//		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
//		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
//
//	ComPtr<ID3DBlob> signature;
//	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr));
//	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
//
//
//
//
//}
//
//void D3D123DApp::Render()
//{
//
//}
//
//void D3D123DApp::Update()
//{
//
//}
//
//void D3D123DApp::Destroy()
//{
//
//}
//
//void D3D123DApp::PopulateCommandLists()
//{
//
//}
//
//void D3D123DApp::WaitForPreviousFrame()
//{
//}
