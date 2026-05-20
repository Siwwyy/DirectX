//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_PASSBASE_H_INCLUDED
#define D3D12_PASSBASE_H_INCLUDED

#include "../pch.h"
#include "../D3D12ShaderCompiler.h"
#include "../Objects/Primitive.h"

class PassBase
{
	DISABLE_COPY(PassBase)

public:

	PassBase() = default;
	~PassBase() = default;

	// Render
	virtual void Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept = 0; // make this class abstract
	virtual HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList) noexcept = 0; // make this class abstract

protected:

	// Pipeline state and root signature	
	ComPtr<ID3D12PipelineState>							PipelineState;
	ComPtr<ID3D12RootSignature>							RootSignature;

	// Shaders
	D3D12ShaderCompiler									ShaderCompiler;
	ComPtr<ID3DBlob>									VertexShader;				// Vertex shader blob
	ComPtr<ID3DBlob>									PixelShader;				// Pixel shader blob
};


#endif // D3D12_PASSBASE_H_INCLUDED