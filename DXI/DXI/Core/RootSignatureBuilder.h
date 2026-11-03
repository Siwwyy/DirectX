//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_ROOTSIGNATUREBUILDER_H_INCLUDED
#define D3D12_ROOTSIGNATUREBUILDER_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"

#include <deque>

// Assembles a root signature a parameter at a time.
//
// Every Add returns the root parameter index to bind that parameter against, which is the
// point of the class. Building the parameter array by hand means keeping a separate enum
// of indices in step with the order of that array, and nothing checks the two agree: swap
// two entries and the binding calls quietly address the wrong slots. Taking the index from
// the builder makes it a consequence of the insertion rather than a second claim about it.
class RootSignatureBuilder
{
public:

	RootSignatureBuilder()	= default;
	~RootSignatureBuilder()	= default;

	// b | A constant buffer read straight out of a GPU address, no descriptor needed
	UINT AddConstantBufferView(const UINT ShaderRegister, const D3D12_SHADER_VISIBILITY Visibility);

	// b | Values living in the root signature itself. Cheapest to change, but they come
	// out of the 64 DWORD budget the whole signature shares, so keep them small.
	UINT AddRootConstants(const UINT ShaderRegister, const UINT Num32BitValues, const D3D12_SHADER_VISIBILITY Visibility);

	// t, u or b | One contiguous range of descriptors out of the bound heap
	UINT AddDescriptorTable(const D3D12_DESCRIPTOR_RANGE_TYPE RangeType,
							const UINT NumDescriptors,
							const UINT BaseShaderRegister,
							const D3D12_SHADER_VISIBILITY Visibility);

	// s | Baked into the signature, so it costs nothing from the budget and needs no heap
	void AddStaticSampler(const UINT ShaderRegister,
						const D3D12_FILTER Filter,
						const D3D12_TEXTURE_ADDRESS_MODE AddressMode,
						const D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_PIXEL);

	// Serializes and creates. Throws with the serializer's own message on failure, which
	// names the offending parameter and is far more use than the HRESULT alone.
	_NODISCARD ComPtr<ID3D12RootSignature> Build(DXDevice * Device, const D3D12_ROOT_SIGNATURE_FLAGS Flags) const;

	// How many DWORDs the parameters so far take out of the 64 the signature gets
	_NODISCARD UINT GetCostInDWords() const noexcept;

private:

	std::vector<D3D12_ROOT_PARAMETER>		Parameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC>	Samplers;

	// A descriptor table holds a pointer to its ranges rather than a copy, so the ranges
	// have to outlive Build and keep their addresses while later parameters are added.
	// A deque is used because push_back on one never moves the elements already in it.
	std::deque<D3D12_DESCRIPTOR_RANGE>		Ranges;
};

#endif // D3D12_ROOTSIGNATUREBUILDER_H_INCLUDED
