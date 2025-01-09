
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_SHADER_COMPILER_H_INCLUDED
#define D3D12_SHADER_COMPILER_H_INCLUDED


#include "pch.h"
#include <dxcapi.h> //for new DX shader compiler API | Has to be included at the end, otherwise got many errors

class D3D12ShaderCompiler
{
public:
	DISABLE_COPY_MOVE(D3D12ShaderCompiler)

	D3D12ShaderCompiler();
	~D3D12ShaderCompiler() = default;

	ComPtr<ID3DBlob> CompileShader(
		LPCWSTR shaderAbsolutePath,
		DxcDefine const * const shaderDefines,
		LPCWSTR entryPoint,
		LPCWSTR targetProfile,
		std::vector<LPCWSTR> arguments = {});	//TODO Add compile arguments for compilation process, See https://simoncoenen.com/blog/programming/graphics/DxcCompiling

	/* [[deprecated]] */ ComPtr<ID3DBlob> CompileShaderD3D(
		LPCWSTR shaderAbsolutePath, 
		D3D_SHADER_MACRO const * shaderDefines,
		LPCSTR entryPoint, 
		LPCSTR targetProfile);

private:

	// IDxc variables
	ComPtr<IDxcLibrary>							library;
	ComPtr<IDxcCompiler3>						compiler;	//Use IDxcCompiler3 instead of IDxcCompiler (because of Deprecatness Compile() function)
	ComPtr<IDxcUtils>							utils;
	ComPtr<IDxcIncludeHandler>					dxcIncludeHandler;

};

#endif // D3D12_SHADER_COMPILER_H_INCLUDED