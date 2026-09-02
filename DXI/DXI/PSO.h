#ifndef PSO_H_INCLUDED
#define PSO_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"


// Base PSO
template<typename Derived>
struct BasePipelineStateDesc
{
	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>							PipelineState;
	ComPtr<ID3D12RootSignature>							RootSignature;

	// Setups the PSO and returns the created PSO object
	HRESULT SetupPSO(ComPtr<DXDevice> Device) noexcept
	{
		return static_cast<Derived*>(this)->SetupPSO(Device);
	}

};

// Graphics PSO
struct GraphicsPipelineStateDesc
{
	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>								PipelineState;
	ComPtr<ID3D12RootSignature>								RootSignature;
	// Shader bytecode
	ComPtr<ID3DBlob>										VertexShaderBlob;
	ComPtr<ID3DBlob>										PixelShaderBlob;
	// Input layout
	std::vector<D3D12_INPUT_ELEMENT_DESC>					InputLayout;
	// Rasterizer state
	D3D12_RASTERIZER_DESC									RasterizerState			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	// Blend state
	D3D12_BLEND_DESC										BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// Depth stencil state
	D3D12_DEPTH_STENCIL_DESC								DepthStencilState		= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	// Render target formats
	std::vector<DXGI_FORMAT>								RenderTargetFormats;
	DXGI_FORMAT												DepthStencilFormat;
	// Sample description
	DXGI_SAMPLE_DESC										SampleDesc;
	// Primitive topology type
	D3D12_PRIMITIVE_TOPOLOGY_TYPE							PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	HRESULT SetupPSO(ComPtr<DXDevice> Device) noexcept
	{
		HRESULT hr = S_OK;
		using namespace Helpers;

		// assertion if needed
		DXASSERT(!InputLayout.empty(), "Input layout cannot be empty.");
		DXASSERT(RootSignature, "Root signature must be set before creating the PSO.");
		DXASSERT(VertexShaderBlob, "Vertex shader blob must be set before creating the PSO.");
		DXASSERT(PixelShaderBlob, "Pixel shader blob must be set before creating the PSO.");
		DXASSERT(!RenderTargetFormats.empty(), "Render target formats cannot be empty.");

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};
		PsoDesc.InputLayout = { InputLayout.data(), static_cast<UINT>(InputLayout.size()) };
		PsoDesc.pRootSignature = RootSignature.Get();
		PsoDesc.VS = CD3DX12_SHADER_BYTECODE(VertexShaderBlob.Get());
		PsoDesc.PS = CD3DX12_SHADER_BYTECODE(PixelShaderBlob.Get());
		PsoDesc.RasterizerState = RasterizerState;
		PsoDesc.BlendState = BlendState;
		PsoDesc.DepthStencilState = DepthStencilState;
		PsoDesc.DSVFormat = DepthStencilFormat;
		PsoDesc.SampleMask = UINT_MAX;
		PsoDesc.SampleDesc = SampleDesc;
		PsoDesc.PrimitiveTopologyType = PrimitiveTopologyType;
		PsoDesc.NumRenderTargets = RenderTargetFormats.size();

		// Render target formats
		for(size_t i = 0; i < RenderTargetFormats.size(); ++i)
		{
			PsoDesc.RTVFormats[i] = RenderTargetFormats[i];
		}
	

		// Create the graphics pipeline state
		ThrowIfFailed(Device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&PipelineState)));

		// Return
		return hr;
	}
};

// Compute PSO
struct ComputePipelineStateDesc
{
	// Pipeline state and root signature
	ComPtr<ID3D12PipelineState>								PipelineState;
	ComPtr<ID3D12RootSignature>								RootSignature;

	// Shader bytecode
	ComPtr<ID3DBlob>										ComputeShaderBlob;

	// Setup PSO
	HRESULT SetupPSO(ComPtr<DXDevice> Device) noexcept
	{
		HRESULT hr = S_OK;
		using namespace Helpers;

		// assertion if needed
		DXASSERT(RootSignature, "Root signature must be set before creating the PSO.");
		DXASSERT(ComputeShaderBlob, "Compute shader blob must be set before creating the PSO.");

		// Empty PSO
		ComPtr<ID3D12PipelineState> PipelineState;

		// Pso Description
		D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc = {};
		PsoDesc.pRootSignature = RootSignature.Get();
		PsoDesc.CS = CD3DX12_SHADER_BYTECODE(ComputeShaderBlob.Get());
		PsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		PsoDesc.NodeMask = 0;
		PsoDesc.CachedPSO = {};

		// Create the compute pipeline state
		ThrowIfFailed(Device->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&PipelineState)));

		// Return
		return hr;
	}
};

// Aliases
using GraphicsPSO	= GraphicsPipelineStateDesc;
using ComputePSO	= ComputePipelineStateDesc;

#endif /* PSO_H_INCLUDED */