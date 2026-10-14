#include "LightPass.h"
#include "../Paths.h"
#include "../GBuffer.h"

#include <pix3.h>

LightPass::LightPass()
{
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"LIGHT_PASS", L"1" });

	const DXIPATHS::FilePath VertexShaderPath = DXIPATHS::GetShaderDir / L"LightPass" / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath = DXIPATHS::GetShaderDir / L"LightPass" / L"pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;
	// String arguments
	//arguments.push_back(L"-enable-16bit-types");
	//arguments.push_back(L"Qstrip_reflect");
	arguments.push_back(L"-Werror");
	arguments.push_back(L"-Wconversion");
	// Defines arguments
	arguments.push_back(DXC_ARG_ALL_RESOURCES_BOUND);
#if DEBUG_MODE
	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);	//-Od
	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);	//-WX
	arguments.push_back(DXC_ARG_DEBUG);					//-Zi
#else
	arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);	//
#endif

	PSO.VertexShaderBlob = DXIShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PSO.PixelShaderBlob = DXIShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

HRESULT LightPass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	//CONSTANTS
	const DXGI_FORMAT						BACK_BUFFER_FORMAT	= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;	//has to match the swap chain
	const DXGI_SAMPLE_DESC					SAMPLE_DESC			= SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup Root Signature
	{
		ComPtr<ID3D12RootSignature> RootSignature;

		// One range covering every G-buffer target, then the shadow map and both sky cubes,
		// so binding everything this pass reads is a single call
		const auto DescriptorRange = CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBufferTargetCount + 3, 0, 0);
		D3D12_DESCRIPTOR_RANGE DescriptorRangesTable[1] = { DescriptorRange };
		const auto DescriptorTable = CreateRootDescriptorTable(_countof(DescriptorRangesTable), DescriptorRangesTable);

		// Constant buffers, the lighting needs the eye position and every light of the scene
		const auto RootCameraCBVDescriptor	= CreateRootDescriptor(0, 1);	// b1 | Per camera
		const auto RootLightsCBVDescriptor	= CreateRootDescriptor(0, 2);	// b2 | Per lights of the scene

		// SRV | The G-buffer
		RootParamHelper ParamGBuffer(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
			DescriptorTable);

		// Camera CBV
		RootParamHelper ParamCameraCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
			RootCameraCBVDescriptor);

		// Lights CBV
		RootParamHelper ParamLightsCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
			RootLightsCBVDescriptor);

		// Create root parameters array | Keep the order, it has to match RootParamIdx
		auto RootParameters = CreateRootParameters({ ParamGBuffer, ParamCameraCBV, ParamLightsCBV });

		// Two static samplers.
		// s0 is point clamp, the G-buffer and the shadow map are read one texel at a time.
		// s1 is linear clamp, the sky is a direction lookup that would look blocky otherwise.
		D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

		samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[0].MipLODBias = 0;
		samplers[0].MaxAnisotropy = 0;
		samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplers[0].MinLOD = 0.0f;
		samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[0].ShaderRegister = 0;
		samplers[0].RegisterSpace = 0;
		samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		samplers[1] = samplers[0];
		samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplers[1].ShaderRegister = 1;

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(RootParameters.size(), // we have 3 root parameters
			RootParameters.data(), // a pointer to the beginning of our root parameters array
			_countof(samplers), // point for the buffers, linear for the sky
			samplers, // a pointer to our static sampler (array)
			// No input assembler here, the fullscreen triangle comes out of SV_VertexID
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		// Initialize Root Signature
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

		PSO.RootSignature = RootSignature;
	}

	// No input layout, the vertex shader builds the triangle out of SV_VertexID

	// Depth | The G-buffer already resolved visibility, so no depth test and no depth buffer
	PSO.DepthStencilState.DepthEnable	= FALSE;
	PSO.DepthStencilState.StencilEnable	= FALSE;
	PSO.DepthStencilFormat				= DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;

	// Render target | Straight into the back buffer
	PSO.RenderTargetFormats.push_back(BACK_BUFFER_FORMAT);
	PSO.SampleDesc						= SAMPLE_DESC;

	// Setup PSO
	PSO.SetupPSO(Device);

	// Return
	return hr;
}

void LightPass::Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	// Nothing to do per primitive, the whole screen is shaded at once by RenderFullscreen()
	DXASSERT(false, "LightPass is a fullscreen pass, call RenderFullscreen instead");
}

void LightPass::RenderFullscreen(DXGraphicsCommandList * const CommandList) noexcept
{
	PIXBeginEvent(CommandList, 0, L"Light Pass");
	{
		// One oversized triangle covering the viewport. The PSO has no input layout,
		// so whatever vertex buffers BasePass left bound are ignored.
		CommandList->DrawInstanced(3, 1, 0, 0);
	}
	PIXEndEvent(CommandList);
}
