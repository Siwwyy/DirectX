#include "SelectionPass.h"
#include "Paths.h"
#include "DXISwapChain.h"
#include "DXIDepthBuffer.h"

#include "PixEvents.h"

SelectionPass::SelectionPass()
{
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"SELECTION_PASS", L"1" });

	const DXIPATHS::FilePath FillVertexShaderPath	= DXIPATHS::GetShaderDir / L"SelectionPass" / L"fill_vertex_shader.hlsl";
	const DXIPATHS::FilePath BoxVertexShaderPath	= DXIPATHS::GetShaderDir / L"SelectionPass" / L"box_vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath		= DXIPATHS::GetShaderDir / L"SelectionPass" / L"pixel_shader.hlsl";

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

	PSO.VertexShaderBlob	= GShaderCompiler.CompileShader(FillVertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PSO.PixelShaderBlob		= GShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);

	// The box has its own vertex shader and shares the pixel shader, which only passes a colour on
	BoxPSO.VertexShaderBlob	= GShaderCompiler.CompileShader(BoxVertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	BoxPSO.PixelShaderBlob	= PSO.PixelShaderBlob;
}

HRESULT SelectionPass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	// Setup Root Signature | Each Add hands back the index to bind that parameter against
	{
		RootSignatureBuilder Builder;

		// Only the vertex shader reads either, the pixel shader just passes a colour on
		PrimitiveCBVIdx	= Builder.AddConstantBufferView(ShaderRegister::PerObject, D3D12_SHADER_VISIBILITY_VERTEX);
		ViewIdx			= Builder.AddRootConstants(ShaderRegister::PerView, SelectionConstantCount, D3D12_SHADER_VISIBILITY_VERTEX);

		// No textures, no samplers. The tint reads the mesh through the input assembler, the box does not.
		PSO.RootSignature = Builder.Build(Device,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
	}

	// Input Layout | The mesh's own vertex buffer, of which only the position is read. A layout
	// may name fewer elements than the buffer holds, the stride comes from the buffer view.
	PSO.InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	PSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Both faces, so a one sided mesh and the inside of an open one tint alike. The surface is
	// the very one the base pass drew, so it ties with the depth it wrote and would flicker
	// in and out: pulled a few units towards the camera, as the tangent frame's lines are.
	PSO.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	PSO.RasterizerState.DepthBias	= -64;

	// Tested against the scene's depth but never written, so the tint cannot occlude anything
	PSO.DepthStencilState.DepthEnable		= TRUE;
	PSO.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
	PSO.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSO.DepthStencilState.StencilEnable		= FALSE;
	PSO.DepthStencilFormat					= DXIDepthBuffer::GetFormat();

	// Laid over what is there rather than replacing it, by the strength in the colour's alpha
	{
		D3D12_RENDER_TARGET_BLEND_DESC& Blend = PSO.BlendState.RenderTarget[0];
		Blend.BlendEnable			= TRUE;
		Blend.SrcBlend				= D3D12_BLEND_SRC_ALPHA;
		Blend.DestBlend				= D3D12_BLEND_INV_SRC_ALPHA;
		Blend.BlendOp				= D3D12_BLEND_OP_ADD;
		Blend.SrcBlendAlpha			= D3D12_BLEND_ONE;
		Blend.DestBlendAlpha		= D3D12_BLEND_ZERO;
		Blend.BlendOpAlpha			= D3D12_BLEND_OP_ADD;
		Blend.RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	// Render target | Straight into the back buffer, over the shaded frame
	PSO.RenderTargetFormats.push_back(DXISwapChain::GetFormat());
	PSO.SampleDesc							= SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup PSO
	PSO.SetupPSO(Device);

	// The box | Triangles as well, every edge is a quad. Nothing to bias against, so unlike the
	// tint it does not test the depth at all and shows through whatever is in front of it. The
	// depth format stays, so both pipelines can be used with one render target binding.
	BoxPSO.RootSignature					= PSO.RootSignature;
	BoxPSO.PrimitiveTopologyType			= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	BoxPSO.RasterizerState.CullMode			= D3D12_CULL_MODE_NONE;
	BoxPSO.DepthStencilState.DepthEnable	= FALSE;
	BoxPSO.DepthStencilState.StencilEnable	= FALSE;
	BoxPSO.DepthStencilFormat				= DXIDepthBuffer::GetFormat();
	BoxPSO.RenderTargetFormats.push_back(DXISwapChain::GetFormat());
	BoxPSO.SampleDesc						= SAMPLER_HELPER::CreateSampler(1, 0);
	BoxPSO.SetupPSO(Device);

	// Return
	return hr;
}

void SelectionPass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	if (Primitive.GetNumIndices() == 0)
	{
		return;
	}

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"%s | selection tint", Primitive.Name.c_str());
	{
		// The same buffers the base pass drew from, so the tint lies exactly on the surface
		CommandList->IASetVertexBuffers(0, 1, &Primitive.VertexFactory.VertexBufferView);
		CommandList->IASetIndexBuffer(&Primitive.VertexFactory.IndexBufferView);
		CommandList->DrawIndexedInstanced(Primitive.GetNumIndices(), 1, 0, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void SelectionPass::RenderBox(DXGraphicsCommandList * const CommandList) noexcept
{
	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"selection box");
	{
		// Twelve edges of six vertices, once. No vertex buffer is bound: the vertex shader works
		// out which corner of which edge it is from the vertex id alone.
		CommandList->DrawInstanced(SELECTION_BOX_VERTICES, 1, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void SelectionPass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	// Nothing here, the overlay is drawn for the selected primitive only
}
