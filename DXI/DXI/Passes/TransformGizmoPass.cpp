#include "TransformGizmoPass.h"
#include "Paths.h"
#include "DXISwapChain.h"

#include "PixEvents.h"

TransformGizmoPass::TransformGizmoPass()
{
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"TRANSFORM_GIZMO_PASS", L"1" });

	const DXIPATHS::FilePath VertexShaderPath = DXIPATHS::GetShaderDir / L"TransformGizmoPass" / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath = DXIPATHS::GetShaderDir / L"TransformGizmoPass" / L"pixel_shader.hlsl";

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

HRESULT TransformGizmoPass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	// Setup Root Signature | The one parameter is the view, the handles belong to no object
	{
		RootSignatureBuilder Builder;

		// Only the vertex shader reads it, the pixel shader just passes a colour on
		ViewIdx = Builder.AddRootConstants(ShaderRegister::PerView, TransformGizmoConstantCount, D3D12_SHADER_VISIBILITY_VERTEX);

		// No textures, no samplers, and no input assembler: the handles come out of SV_VertexID
		PSO.RootSignature = Builder.Build(Device,
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
	}

	// No input layout, the vertex shader builds everything out of SV_VertexID

	// Triangles. The lines are screen space quads, since D3D12 has no line wider than a pixel,
	// and the arrow heads are triangles to begin with.
	PSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Neither side matters, and the quads may come out either way round
	PSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// No depth at all, neither tested nor written. The handles have to be reachable wherever the
	// object is, and with no depth buffer bound they also leave it as they found it.
	PSO.DepthStencilState.DepthEnable	= FALSE;
	PSO.DepthStencilState.StencilEnable	= FALSE;
	PSO.DepthStencilFormat				= DXGI_FORMAT_UNKNOWN;

	// Render target | Straight into the back buffer, over the shaded frame
	PSO.RenderTargetFormats.push_back(DXISwapChain::GetFormat());
	PSO.SampleDesc						= SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup PSO
	PSO.SetupPSO(Device);

	// Return
	return hr;
}

void TransformGizmoPass::RenderGizmo(DXGraphicsCommandList * const CommandList, const TransformMode Mode) noexcept
{
	// Arrows are 27 vertices, three nine vertex arrows. Rings are 1170: three rings of 384, and
	// the three lines they turn about. One draw either way.
	const UINT VertexCount = (Mode == TransformMode::Move) ? TRANSFORM_GIZMO_MOVE_VERTEX_COUNT : TRANSFORM_GIZMO_ROTATE_VERTEX_COUNT;

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"%s handles", Mode == TransformMode::Move ? L"Move" : L"Rotate");
	{
		CommandList->DrawInstanced(VertexCount, 1, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void TransformGizmoPass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	// Nothing here, the handles are drawn once, not per object
}

void TransformGizmoPass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	// Nothing here either
}
