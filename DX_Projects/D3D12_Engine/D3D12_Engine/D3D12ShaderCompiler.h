#pragma once


#include "D3D12Helpers.h"
#include <dxcapi.h> //for new DX shader compiler API | Has to be included at the end, otherwise got many errors

class D3D12ShaderCompiler
{
public:
	DISABLE_COPY(D3D12ShaderCompiler)

	D3D12ShaderCompiler();
	~D3D12ShaderCompiler() = default;

	ComPtr<ID3DBlob> CompileShader(
		LPCWSTR shaderAbsolutePath,
		const DxcDefine* shaderDefines,
		LPCWSTR entryPoint,
		LPCWSTR targetProfile);	//TODO Add compile arguments for compilation process, See https://simoncoenen.com/blog/programming/graphics/DxcCompiling

	[[deprecated]] ComPtr<ID3DBlob> CompileShaderD3D(
		LPCWSTR shaderAbsolutePath, 
		const D3D_SHADER_MACRO* shaderDefines,
		LPCSTR entryPoint, 
		LPCSTR targetProfile);

private:

	// IDxc variables
	ComPtr<IDxcLibrary>							library;
	ComPtr<IDxcCompiler>						compiler;

};