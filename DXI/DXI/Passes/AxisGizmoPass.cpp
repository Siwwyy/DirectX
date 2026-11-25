#include "AxisGizmoPass.h"
#include "Paths.h"
#include "DXISwapChain.h"
#include "DXIDepthBuffer.h"

#include "PixEvents.h"

AxisGizmoPass::AxisGizmoPass()
{
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"AXIS_GIZMO_PASS", L"1" });

	const DXIPATHS::FilePath VertexShaderPath = DXIPATHS::GetShaderDir / L"AxisGizmoPass" / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath = DXIPATHS::GetShaderDir / L"AxisGizmoPass" / L"pixel_shader.hlsl";

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

	PSO.VertexShaderBlob	= GShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PSO.PixelShaderBlob		= GShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

HRESULT AxisGizmoPass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	// Setup Root Signature | Each Add hands back the index to bind that parameter against
	{
		RootSignatureBuilder Builder;

		// Only the vertex shader reads either, the pixel shader just passes a colour on
		PrimitiveCBVIdx	= Builder.AddConstantBufferView(ShaderRegister::PerObject, D3D12_SHADER_VISIBILITY_VERTEX);
		ViewIdx			= Builder.AddRootConstants(ShaderRegister::PerView, AxisGizmoConstantCount, D3D12_SHADER_VISIBILITY_VERTEX);

		// No textures, no samplers, and no input assembler: the arrows come out of SV_VertexID
		PSO.RootSignature = Builder.Build(Device,
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
	}

	// No input layout, the vertex shader builds everything out of SV_VertexID

	// Triangles. The shafts are screen space quads, since D3D12 has no line wider than a
	// pixel, and the heads are triangles to begin with.
	PSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Nothing here lies in a surface, the arrows start at an origin and leave it, so unlike
	// the tangent frame there is no tie with the depth to break and no bias to apply
	PSO.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;

	// Tested against the scene's depth but never written, so the overlay cannot occlude
	// anything and the arrows do not hide each other
	PSO.DepthStencilState.DepthEnable		= TRUE;
	PSO.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
	PSO.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSO.DepthStencilState.StencilEnable		= FALSE;
	PSO.DepthStencilFormat					= DXIDepthBuffer::GetFormat();

	// Render target | Straight into the back buffer, over the shaded frame
	PSO.RenderTargetFormats.push_back(DXISwapChain::GetFormat());
	PSO.SampleDesc							= SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup PSO
	PSO.SetupPSO(Device);

	// Same thing with the depth test off. Everything else stays, including the depth format,
	// so the same target binding serves both.
	OnTopPSO								= PSO;
	OnTopPSO.DepthStencilState.DepthEnable	= FALSE;
	OnTopPSO.SetupPSO(Device);

	// Return
	return hr;
}

void AxisGizmoPass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"%s | origin axes", Primitive.Name.c_str());
	{
		// Three arrows of nine vertices each, once. No vertex buffer is bound: the vertex
		// shader works out which part of which arrow it is from the vertex id alone.
		CommandList->DrawInstanced(AXIS_GIZMO_VERTEX_COUNT, 1, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void AxisGizmoPass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	// Nothing here, the overlay is drawn per primitive
}
