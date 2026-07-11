#include "D3D12ShaderCompiler.h"

#include <cassert>
#include <D3Dcompiler.h>

using namespace Helpers;

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary,	IID_PPV_ARGS(&Library)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler,	IID_PPV_ARGS(&Compiler)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils,		IID_PPV_ARGS(&Utils)));
	ThrowIfFailed(Library->CreateIncludeHandler(&DxcIncludeHandler));
}

// Old D3D HLSL Shader compiler
ComPtr<ID3DBlob> D3D12ShaderCompiler::CompileShaderD3D(
	LPCWSTR ShaderAbsolutePath, 
	D3D_SHADER_MACRO const * ShaderDefines,
	LPCSTR EntryPoint, 
	LPCSTR TargetProfile)
{
	DXASSERT(ShaderAbsolutePath || EntryPoint || TargetProfile, "Shader path, entry point or target profile has been not provided");

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | 
						D3DCOMPILE_SKIP_OPTIMIZATION | 
						D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
	UINT compileFlags = 0;
#endif

	ComPtr<ID3DBlob> code;
	ComPtr<ID3DBlob> errorBlob;
	auto hr = D3DCompileFromFile(
		ShaderAbsolutePath,
		ShaderDefines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint, 
		TargetProfile,
		compileFlags,
		0, 
		&code, 
		&errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)
		{
			OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
		}
		if (code)
		{
			code->Release();
		}
	}
	return code;
}