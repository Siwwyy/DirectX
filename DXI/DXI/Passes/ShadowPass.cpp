#include "ShadowPass.h"
#include "Paths.h"
#include "ShadowAtlas.h"

#include "PixEvents.h"

ShadowPass::ShadowPass()
{
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"SHADOW_PASS", L"1" });

	const DXIPATHS::FilePath VertexShaderPath		= DXIPATHS::GetShaderDir / L"ShadowPass" / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath DistanceVSPath			= DXIPATHS::GetShaderDir / L"ShadowPass" / L"distance_vertex_shader.hlsl";
	const DXIPATHS::FilePath DistancePSPath			= DXIPATHS::GetShaderDir / L"ShadowPass" / L"distance_pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;
	// String arguments
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

	// Depth only, so this one has no pixel shader at all
	PSO.VertexShaderBlob			= GShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);

	// Distance needs a pixel shader, that is the whole point of it
	DistancePSO.VertexShaderBlob	= GShaderCompiler.CompileShader(DistanceVSPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	DistancePSO.PixelShaderBlob		= GShaderCompiler.CompileShader(DistancePSPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

HRESULT ShadowPass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	//CONSTANTS
	const DXGI_SAMPLE_DESC SAMPLE_DESC = SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup Root Signature | Per object matrices plus whichever view is being rendered
	{
		RootSignatureBuilder Builder;

		PrimitiveCBVIdx	= Builder.AddConstantBufferView(ShaderRegister::PerObject, D3D12_SHADER_VISIBILITY_ALL);

		// The vertex shader projects with it, the pixel shader needs the range to
		// normalize the distance, so this one is visible to both
		ShadowViewIdx	= Builder.AddRootConstants(ShaderRegister::PerView, ShadowViewConstantCount, D3D12_SHADER_VISIBILITY_ALL);

		// No static samplers, a shadow pass samples nothing
		const auto RootSignature = Builder.Build(Device,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		// Both pipelines render the same geometry from the same view, only the targets differ
		PSO.RootSignature			= RootSignature;
		DistancePSO.RootSignature	= RootSignature;
	}

	// Input Layout | Same vertex format as the base pass, only the position is read.
	// Tangent is declared because VSInput is shared, the PSO would reject a shader that
	// asks for an element the layout does not provide.
	const std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Front face culling trades surface acne for a bit of peter panning, which is the
	// easier artefact to live with on closed geometry like these cubes
	PSO.InputLayout					= InputLayout;
	PSO.DepthStencilFormat			= ShadowAtlas::GetDepthFormat();
	PSO.SampleDesc					= SAMPLE_DESC;
	PSO.RasterizerState.CullMode	= D3D12_CULL_MODE_FRONT;
	PSO.SetupPSO(Device);

	// Same thing, but writing distance into a colour target as well
	DistancePSO.InputLayout					= InputLayout;
	DistancePSO.DepthStencilFormat			= ShadowAtlas::GetDepthFormat();
	DistancePSO.SampleDesc					= SAMPLE_DESC;
	DistancePSO.RasterizerState.CullMode	= D3D12_CULL_MODE_FRONT;
	DistancePSO.RenderTargetFormats.push_back(ShadowAtlas::GetDistanceFormat());
	DistancePSO.SetupPSO(Device);

	// Return
	return hr;
}

void ShadowPass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	// Named per primitive, since this pass draws the whole scene once per light view and a
	// capture is unreadable otherwise. No material here: a shadow view only writes depth
	// or distance, so which material the surface carries makes no difference.
	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"%s | %u indices",
					Primitive.Name.c_str(),
					Primitive.GetNumIndices());
	{
		CommandList->IASetVertexBuffers(0, 1, &Primitive.VertexFactory.VertexBufferView);
		CommandList->IASetIndexBuffer(&Primitive.VertexFactory.IndexBufferView);
		CommandList->DrawIndexedInstanced(Primitive.GetNumIndices(), 1, 0, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void ShadowPass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	// Nothing here so far...
}
