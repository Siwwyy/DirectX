#include "BasePass.h"

#include <pix3.h>

BasePass::BasePass()
{
	// Init shaders
	
	// Shaders compilation
	DXShaderDefines<1> ShaderDefines;
	ShaderDefines.AddDefine({ L"BASE_PASS", L"1" });

	constexpr const wchar_t* VertexShaderPath = L"shaders//BasePass//base_pass_vertex_shader.hlsl";
	constexpr const wchar_t* PixelShaderPath = L"shaders//BasePass//base_pass_pixel_shader.hlsl";

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

	VertexShader = ShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PixelShader = ShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

HRESULT BasePass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	//CONSTANTS
	const DXGI_FORMAT						BACK_BUFFER_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT						DEPTH_STENCIL_FORMAT	= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format
	const DXGI_SAMPLE_DESC					SAMPLE_DESC				= SAMPLER_HELPER::CreateSampler(1, 0);

	// Input Element Desc
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,		0,		0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32_FLOAT,			0,		16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,			0,		28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Pipeline state object (PSO)
	{
		// CREATE ROOT SIGNATURE
		// create a root descriptor, which explains where to find the data for this root parameter
		const auto RootCBVDescriptor = CreateRootDescriptor(0, 0);

		// Create Param
		// CBV
		RootParamHelper ParamCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
								D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX,
								RootCBVDescriptor);

		// Create root parameters array
		auto RootParameters = CreateRootParameters({ ParamCBV });

		// create a static sampler
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias			= 0;
		sampler.MaxAnisotropy		= 0;
		sampler.ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor			= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD				= 0.0f;
		sampler.MaxLOD				= D3D12_FLOAT32_MAX;
		sampler.ShaderRegister		= 0;
		sampler.RegisterSpace		= 0;
		sampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(RootParameters.size(), // we have 2 root parameters
			RootParameters.data(), // a pointer to the beginning of our root parameters array
			1, // we have one static sampler
			&sampler, // a pointer to our static sampler (array)
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		// Initialize Root Signature
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));
	}

	// Depth Stencil Desc
	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = 1;
	DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	DepthStencilDesc.StencilEnable = 0;
	DepthStencilDesc.StencilWriteMask = 0;
	DepthStencilDesc.StencilReadMask = 0;
	DepthStencilDesc.FrontFace = {};
	DepthStencilDesc.BackFace = {};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};
	PsoDesc.InputLayout				= { InputElementDesc, _countof(InputElementDesc) };
	PsoDesc.pRootSignature			= RootSignature.Get();
	PsoDesc.VS						= CD3DX12_SHADER_BYTECODE(VertexShader.Get());
	PsoDesc.PS						= CD3DX12_SHADER_BYTECODE(PixelShader.Get());
	PsoDesc.RasterizerState			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PsoDesc.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.DepthStencilState		= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PsoDesc.DSVFormat				= DEPTH_STENCIL_FORMAT;
	PsoDesc.SampleMask				= UINT_MAX;
	PsoDesc.SampleDesc				= SAMPLE_DESC;
	PsoDesc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PsoDesc.NumRenderTargets		= 1;
	PsoDesc.RTVFormats[0]			= BACK_BUFFER_FORMAT;
	PsoDesc.SampleDesc.Count		= 1;
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&PipelineState)));

	// Return
	return NOERROR;
}

void BasePass::Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{
	PIXBeginEvent(CommandList, 0, Primitive.Name.c_str());
	{
		//CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferUploadHeaps[CurrentFrameIdx]->GetGPUVirtualAddress() + (0 * ConstantBufferPerObjectSize));
		CommandList->IASetVertexBuffers(0, 1, &Primitive.VertexFactory.VertexBufferView); // set the vertex buffer (using the vertex buffer view)
		CommandList->IASetIndexBuffer(&Primitive.VertexFactory.IndexBufferView);
		CommandList->DrawIndexedInstanced(Primitive.GetNumIndices(), 1, 0, 0, 0); // draw cube
	}
	PIXEndEvent(CommandList);
}