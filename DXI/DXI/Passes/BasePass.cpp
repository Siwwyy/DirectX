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

	PSO.VertexShaderBlob	= DXIShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments, &ShaderDefines);
	PSO.PixelShaderBlob		= DXIShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments, &ShaderDefines);
}

HRESULT BasePass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	HRESULT hr = S_OK;

	// Namespaces
	using namespace Helpers;

	//CONSTANTS
	const DXGI_FORMAT						BACK_BUFFER_FORMAT		= DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
	const DXGI_FORMAT						DEPTH_STENCIL_FORMAT	= DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;					//depth stencil format
	const DXGI_SAMPLE_DESC					SAMPLE_DESC				= SAMPLER_HELPER::CreateSampler(1, 0);

	// Setup Root Signature
	{
		ComPtr<ID3D12RootSignature> RootSignature;
		// CREATE ROOT SIGNATURE
		// create a root descriptor, which explains where to find the data for this root parameter
		const auto RootCBVDescriptor = CreateRootDescriptor(0, 0);

		// create a descriptor table
		// create a descriptor range (descriptor table) and fill it out
		// this is a range of descriptors inside a descriptor heap
		const auto DescriptorRange = CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		D3D12_DESCRIPTOR_RANGE DescriptorRangesTable[1] = { DescriptorRange };
		const auto DescriptorTable = CreateRootDescriptorTable(_countof(DescriptorRangesTable), DescriptorRangesTable);

		// Create Param
		// CBV
		RootParamHelper ParamCBV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX,
			RootCBVDescriptor);

		// SRV
		RootParamHelper ParamSRV(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL,
			DescriptorTable);

		// Create root parameters array
		auto RootParameters = CreateRootParameters({ ParamCBV, ParamSRV });

		// create a static sampler
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

		PSO.RootSignature = RootSignature;
	}

	// Input Layout
	PSO.InputLayout = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Other things
	PSO.RenderTargetFormats.push_back(BACK_BUFFER_FORMAT);
	PSO.DepthStencilFormat	= DEPTH_STENCIL_FORMAT;
	PSO.SampleDesc			= SAMPLE_DESC;
	
	// Setup PSO
	PSO.SetupPSO(Device);

	// Return
	return hr;
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