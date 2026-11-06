#include "RootSignatureBuilder.h"

// Usings
using Helpers::ThrowIfFailed;

UINT RootSignatureBuilder::AddConstantBufferView(const UINT ShaderRegister, const D3D12_SHADER_VISIBILITY Visibility)
{
	D3D12_ROOT_PARAMETER Parameter			= {};
	Parameter.ParameterType					= D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameter.ShaderVisibility				= Visibility;
	Parameter.Descriptor.ShaderRegister		= ShaderRegister;
	Parameter.Descriptor.RegisterSpace		= 0;

	Parameters.push_back(Parameter);
	return static_cast<UINT>(Parameters.size() - 1);
}

UINT RootSignatureBuilder::AddRootConstants(const UINT ShaderRegister, const UINT Num32BitValues, const D3D12_SHADER_VISIBILITY Visibility)
{
	DXASSERT(Num32BitValues > 0, "Root constants with no values is not a parameter");

	D3D12_ROOT_PARAMETER Parameter			= {};
	Parameter.ParameterType					= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameter.ShaderVisibility				= Visibility;
	Parameter.Constants.ShaderRegister		= ShaderRegister;
	Parameter.Constants.RegisterSpace		= 0;
	Parameter.Constants.Num32BitValues		= Num32BitValues;

	Parameters.push_back(Parameter);
	return static_cast<UINT>(Parameters.size() - 1);
}

UINT RootSignatureBuilder::AddDescriptorTable(const D3D12_DESCRIPTOR_RANGE_TYPE RangeType,
											const UINT NumDescriptors,
											const UINT BaseShaderRegister,
											const D3D12_SHADER_VISIBILITY Visibility)
{
	DXASSERT(NumDescriptors > 0, "An empty descriptor table binds nothing");

	D3D12_DESCRIPTOR_RANGE Range					= {};
	Range.RangeType									= RangeType;
	Range.NumDescriptors							= NumDescriptors;
	Range.BaseShaderRegister						= BaseShaderRegister;
	Range.RegisterSpace								= 0;
	Range.OffsetInDescriptorsFromTableStart			= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	Ranges.push_back(Range);

	D3D12_ROOT_PARAMETER Parameter					= {};
	Parameter.ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameter.ShaderVisibility						= Visibility;
	Parameter.DescriptorTable.NumDescriptorRanges	= 1;
	Parameter.DescriptorTable.pDescriptorRanges		= &Ranges.back();

	Parameters.push_back(Parameter);
	return static_cast<UINT>(Parameters.size() - 1);
}

void RootSignatureBuilder::AddStaticSampler(const UINT ShaderRegister,
											const D3D12_FILTER Filter,
											const D3D12_TEXTURE_ADDRESS_MODE AddressMode,
											const D3D12_SHADER_VISIBILITY Visibility)
{
	D3D12_STATIC_SAMPLER_DESC Sampler	= {};
	Sampler.Filter						= Filter;
	Sampler.AddressU					= AddressMode;
	Sampler.AddressV					= AddressMode;
	Sampler.AddressW					= AddressMode;
	Sampler.MipLODBias					= 0;
	Sampler.MaxAnisotropy				= 0;
	Sampler.ComparisonFunc				= D3D12_COMPARISON_FUNC_NEVER;
	Sampler.BorderColor					= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	Sampler.MinLOD						= 0.0f;
	Sampler.MaxLOD						= D3D12_FLOAT32_MAX;
	Sampler.ShaderRegister				= ShaderRegister;
	Sampler.RegisterSpace				= 0;
	Sampler.ShaderVisibility			= Visibility;

	Samplers.push_back(Sampler);
}

UINT RootSignatureBuilder::GetCostInDWords() const noexcept
{
	UINT Cost = 0;

	for (const auto& Parameter : Parameters)
	{
		switch (Parameter.ParameterType)
		{
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			Cost += Parameter.Constants.Num32BitValues;
			break;
		case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			Cost += 1;		// just the handle
			break;
		default:
			Cost += 2;		// a raw GPU address
			break;
		}
	}

	return Cost;
}

ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(DXDevice * Device, const D3D12_ROOT_SIGNATURE_FLAGS Flags) const
{
	DXASSERT(Device, "Device can not be null when building a root signature");
	//DXASSERT(GetCostInDWords() <= 64, "Root signature is over the 64 DWORD budget, move something into a constant buffer");

	CD3DX12_ROOT_SIGNATURE_DESC Desc;
	Desc.Init(static_cast<UINT>(Parameters.size()),
			Parameters.empty() ? nullptr : Parameters.data(),
			static_cast<UINT>(Samplers.size()),
			Samplers.empty() ? nullptr : Samplers.data(),
			Flags);

	ComPtr<ID3DBlob> Signature;
	ComPtr<ID3DBlob> Error;

	const auto SerializeResult = D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);
	if (FAILED(SerializeResult) && Error)
	{
		// The blob says which parameter is wrong, the HRESULT only says that one is
		DXLOG("Root signature serialization failed: %s\n", static_cast<const char*>(Error->GetBufferPointer()))
	}
	ThrowIfFailed(SerializeResult);

	ComPtr<ID3D12RootSignature> RootSignature;
	ThrowIfFailed(Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

	return RootSignature;
}
