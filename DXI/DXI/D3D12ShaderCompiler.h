
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_SHADER_COMPILER_H_INCLUDED
#define D3D12_SHADER_COMPILER_H_INCLUDED


#include "pch.h"
#include "D3D12Helpers.h"
#include <dxcapi.h> //for new DX shader compiler API | Has to be included at the end, otherwise got many errors

class D3D12ShaderCompiler
{
public:
	DISABLE_COPY_MOVE(D3D12ShaderCompiler)

	D3D12ShaderCompiler();
	~D3D12ShaderCompiler() = default;

	ComPtr<ID3DBlob> CompileShader(
		LPCWSTR ShaderAbsolutePath,
		DxcDefine const * const ShaderDefines,
		LPCWSTR EntryPoint,
		LPCWSTR TargetProfile,
		std::vector<LPCWSTR> Arguments = {});	//TODO Add compile arguments for compilation process, See https://simoncoenen.com/blog/programming/graphics/DxcCompiling

	/* [[deprecated]] */ ComPtr<ID3DBlob> CompileShaderD3D(
		LPCWSTR ShaderAbsolutePath, 
		D3D_SHADER_MACRO const * ShaderDefines,
		LPCSTR EntryPoint, 
		LPCSTR TargetProfile);

private:

	// IDxc variables
	ComPtr<IDxcLibrary>							Library;
	ComPtr<IDxcCompiler3>						Compiler;	//Use IDxcCompiler3 instead of IDxcCompiler (because of Deprecatness Compile() function)
	ComPtr<IDxcUtils>							Utils;
	ComPtr<IDxcIncludeHandler>					DxcIncludeHandler;

};

#endif // D3D12_SHADER_COMPILER_H_INCLUDED