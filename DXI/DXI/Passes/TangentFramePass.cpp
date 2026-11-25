#include "TangentFramePass.h"
#include "Paths.h"
#include "DXISwapChain.h"
#include "DXIDepthBuffer.h"

#include "PixEvents.h"

TangentFramePass::TangentFramePass()
{
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"TANGENT_FRAME_PASS", L"1" });

	const DXIPATHS::FilePath VertexShaderPath = DXIPATHS::GetShaderDir / L"TangentFramePass" / L"vertex_shader.hlsl";
	const DXIPATHS::FilePath PixelShaderPath = DXIPATHS::GetShaderDir / L"TangentFramePass" / L"pixel_shader.hlsl";

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

HRESULT TangentFramePass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	// Setup Root Signature | Each Add hands back the index to bind that parameter against
	{
		RootSignatureBuilder Builder;

		// Only the vertex shader reads either, the pixel shader just passes a colour on
		PrimitiveCBVIdx	= Builder.AddConstantBufferView(ShaderRegister::PerObject, D3D12_SHADER_VISIBILITY_VERTEX);
		ViewIdx			= Builder.AddRootConstants(ShaderRegister::PerView, TangentFrameConstantCount, D3D12_SHADER_VISIBILITY_VERTEX);

		// No textures, no samplers
		PSO.RootSignature = Builder.Build(Device,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
	}

	// Input Layout | The same vertex buffer the base pass draws, read as per instance data.
	// Step rate 1 hands one whole vertex to each instance, and the six vertices of an
	// instance then all see that same vertex. TEXCOORD is left out: nothing reads it, and a
	// layout may carry fewer elements than the buffer holds.
	PSO.InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	// Triangles, not lines. D3D12 has no line wider than one pixel, so every line is a quad
	// the vertex shader stretches to the width asked for. The default topology type is
	// triangle already, it is spelled out because the pass used to say line.
	PSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// The vector lying in the surface is coplanar with it, so it ties with the depth the base
	// pass wrote and would flicker in and out. Pulled a few units towards the camera, which for
	// a float depth buffer is a relative step, so it holds at any distance.
	PSO.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
	PSO.RasterizerState.DepthBias	= -64;

	// Tested against the scene's depth but never written, so the overlay cannot occlude
	// anything and the lines do not hide each other
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

	// Same thing with the depth test off, for seeing the frame through geometry. Everything
	// else stays, including the depth format, so the same target binding serves both.
	OnTopPSO								= PSO;
	OnTopPSO.DepthStencilState.DepthEnable	= FALSE;
	OnTopPSO.SetupPSO(Device);

	// Return
	return hr;
}

void TangentFramePass::RenderPrimitive(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	const UINT VertexCount = Primitive.VertexFactory.GetNumVertices();
	if (VertexCount == 0)
	{
		return;
	}

	// Every vertex, ignoring the share the panel asks for, when the mesh is small enough to
	// not be worth sampling or is a shape that always shows all of it: the built in cube and plane.
	// The shader cannot count the vertices of a mesh, so the verdict is made here.
	const bool bShowAll = VertexCount <= TANGENT_FRAME_SHOW_ALL_VERTEX_LIMIT
						|| Primitive.AlwaysShowsAllFrameVertices();

	DXI_PIX_BEGIN(CommandList, DXIPIXCOLORS::DRAW, L"%s | %u vertices%s",
					Primitive.Name.c_str(),
					VertexCount,
					bShowAll ? L" | all shown" : L"");
	{
		// Written on its own for every draw, everything else in the constants is shared
		CommandList->SetGraphicsRoot32BitConstant(ViewIdx, bShowAll ? 1u : 0u, TangentFrameShowAllOffset);

		// The vertex buffer goes in as it is. Which of it is read as a vertex and which as
		// an instance is the input layout's business, not the buffer's.
		CommandList->IASetVertexBuffers(0, 1, &Primitive.VertexFactory.VertexBufferView);

		// Three quads of six corners each, once per vertex of the mesh
		CommandList->DrawInstanced(TANGENT_FRAME_VERTICES_PER_VERTEX, VertexCount, 0, 0);
	}
	DXI_PIX_END(CommandList);
}

void TangentFramePass::RenderFullScreen(DXGraphicsCommandList * const CommandList) noexcept
{
	// Nothing here, the overlay is drawn per primitive
}
