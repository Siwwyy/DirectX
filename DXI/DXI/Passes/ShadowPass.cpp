#include "ShadowPass.h"
#include "../Paths.h"
#include "../ShadowAtlas.h"

#include <pix3.h>

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
	PSO.VertexShaderBlob			= DXIShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);

	// Distance needs a pixel shader, that is the whole point of it
	DistancePSO.VertexShaderBlob	= DXIShaderCompiler.CompileShader(DistanceVSPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	DistancePSO.PixelShaderBlob		= DXIShaderCompiler.CompileShader(DistancePSPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
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
		ComPtr<ID3D12RootSignature> RootSignature;

		const auto RootCBVDescriptor	= CreateRootDescriptor(0, 0);							// b0 | Per object
		const auto RootViewConstants	= CreateRootConstant(0, 1, ShadowViewConstantCount);		// b1 | Per view

		RootParamHelper ParamCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL,
			RootCBVDescriptor);

		// The vertex shader projects with it, the pixel shader needs the range to
		// normalize the distance, so this one is visible to both
		RootParamHelper ParamViewConstants(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL,
			RootViewConstants);

		// Keep the order, it has to match RootParamIdx
		auto RootParameters = CreateRootParameters({ ParamCBV, ParamViewConstants });

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(RootParameters.size(),	// we have 2 root parameters
			RootParameters.data(),						// a pointer to the beginning of our root parameters array
			0,											// no static samplers, a shadow pass samples nothing
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		// Initialize Root Signature
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

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
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

void ShadowPass::Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	CommandList->IASetVertexBuffers(0, 1, &Primitive.VertexFactory.VertexBufferView);
	CommandList->IASetIndexBuffer(&Primitive.VertexFactory.IndexBufferView);
	CommandList->DrawIndexedInstanced(Primitive.GetNumIndices(), 1, 0, 0, 0);
}
