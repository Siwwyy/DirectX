#include "LightPass.h"
#include "../Paths.h"
#include "../GBuffer.h"
#include "../ShadowAtlas.h"

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

	// Setup Root Signature | Each Add hands back the index to bind that parameter against
	{
		RootSignatureBuilder Builder;

		// One range covering every G-buffer target, then the shadow atlas and both sky
		// cubes, so binding everything this pass reads is a single call
		GBufferTableIdx	= Builder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GetSrvCount(), ShaderRegister::TableBase, D3D12_SHADER_VISIBILITY_PIXEL);

		// Constant buffers, the lighting needs the eye position and every light of the scene
		CameraCBVIdx	= Builder.AddConstantBufferView(ShaderRegister::PerView, D3D12_SHADER_VISIBILITY_PIXEL);
		LightsCBVIdx	= Builder.AddConstantBufferView(ShaderRegister::PerLights, D3D12_SHADER_VISIBILITY_PIXEL);

		// s0 is point clamp, the G-buffer and the shadow map are read one texel at a time.
		// s1 is linear clamp, the sky is a direction lookup that would look blocky otherwise.
		Builder.AddStaticSampler(ShaderRegister::SamplerBase,		D3D12_FILTER_MIN_MAG_MIP_POINT,		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		Builder.AddStaticSampler(ShaderRegister::SamplerBase + 1,	D3D12_FILTER_MIN_MAG_MIP_LINEAR,	D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		PSO.RootSignature = Builder.Build(Device,
			// No input assembler here, the fullscreen triangle comes out of SV_VertexID
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
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

void LightPass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	// Nothing to do per primitive, the whole screen is shaded at once by RenderFullscreen()
	DXASSERT(false, "LightPass is a fullscreen pass, call RenderFullscreen instead");
}

void LightPass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	PIXBeginEvent(CommandList, 0, L"Light Pass");
	{
		// One oversized triangle covering the viewport. The PSO has no input layout,
		// so whatever vertex buffers BasePass left bound are ignored.
		CommandList->DrawInstanced(3, 1, 0, 0);
	}
	PIXEndEvent(CommandList);
}
