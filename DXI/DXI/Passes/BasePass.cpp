#include "BasePass.h"
#include "Paths.h"
#include "GBuffer.h"
#include "Material.h"

#include "PixEvents.h"

BasePass::BasePass()
{
	// Init shaders
	
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"BASE_PASS", L"1" });

	const DXIPATHS::FilePath VertexShaderPath = DXIPATHS::GetShaderDir / L"BasePass" / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath = DXIPATHS::GetShaderDir / L"BasePass" / L"pixel_shader.hlsl";

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

	PSO.VertexShaderBlob	= GShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PSO.PixelShaderBlob		= GShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

HRESULT BasePass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	//CONSTANTS
	const DXGI_FORMAT						DEPTH_STENCIL_FORMAT	= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format
	const DXGI_SAMPLE_DESC					SAMPLE_DESC				= SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup Root Signature | Each Add hands back the index to bind that parameter against
	{
		RootSignatureBuilder Builder;

		PrimitiveCBVIdx		= Builder.AddConstantBufferView(ShaderRegister::PerObject, D3D12_SHADER_VISIBILITY_VERTEX);

		// One range covering every texture slot of a material, so binding a material is a single call
		MaterialTableIdx	= Builder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MaterialTextureSlotCount, ShaderRegister::TableBase, D3D12_SHADER_VISIBILITY_PIXEL);

		// Debug mode | A single value, so it rides in the root signature itself rather
		// than in a constant buffer of its own
		DebugModeIdx		= Builder.AddRootConstants(ShaderRegister::PerView, 1, D3D12_SHADER_VISIBILITY_PIXEL);

		// Linear on all three axes, including mip. Material textures carry a full mip
		// chain now, and point filtering between mips would make them pop as they change.
		Builder.AddStaticSampler(ShaderRegister::SamplerBase, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

		PSO.RootSignature = Builder.Build(Device,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |	// shader stages are denied below for better performance
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
	}

	// Input Layout | Matches VertexPositionuvNormalTangent
	PSO.InputLayout = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "uv", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Render targets | One per G-buffer target, in the GBufferTarget order
	for (UINT TargetIdx = 0; TargetIdx < GBufferTargetCount; ++TargetIdx)
	{
		PSO.RenderTargetFormats.push_back(GBuffer::GetFormat(static_cast<GBufferTarget>(TargetIdx)));
	}

	// Other things
	PSO.DepthStencilFormat	= DEPTH_STENCIL_FORMAT;
	PSO.SampleDesc			= SAMPLE_DESC;
	
	// Setup PSO
	PSO.SetupPSO(Device);

	// Same thing with culling off, for two sided materials. The cull mode is part of the
	// pipeline state, so a two sided surface needs its own PSO rather than a draw flag.
	TwoSidedPSO								= PSO;
	TwoSidedPSO.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	TwoSidedPSO.SetupPSO(Device);

	// Return
	return hr;
}

void BasePass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	// The material is worth carrying here: this is the pass that binds it, so a capture
	// showing which maps a draw sampled is what answers "why does that surface look wrong".
	const auto * const PrimitiveMaterial = Primitive.GetMaterial();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"%s | %s | %u indices",
					Primitive.Name.c_str(),
					PrimitiveMaterial ? PrimitiveMaterial->GetName().c_str() : L"no material",
					Primitive.GetNumIndices());
	{
		CommandList->IASetVertexBuffers(0, 1, &Primitive.VertexFactory.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		CommandList->IASetIndexBuffer(&Primitive.VertexFactory.IndexBufferView);
		CommandList->DrawIndexedInstanced(Primitive.GetNumIndices(), 1, 0, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void BasePass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	// nothing here, so far...
}