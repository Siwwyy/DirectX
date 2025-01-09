#include "D3D12ShaderCompiler.h"

#include <cassert>
#include <D3Dcompiler.h>

using namespace Helpers;

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary,	IID_PPV_ARGS(&library)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler,	IID_PPV_ARGS(&compiler)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils,		IID_PPV_ARGS(&utils)));
	ThrowIfFailed(library->CreateIncludeHandler(&dxcIncludeHandler));
}

ComPtr<ID3DBlob> D3D12ShaderCompiler::CompileShader(
	LPCWSTR shaderAbsolutePath,
	DxcDefine const * const shaderDefines,
	LPCWSTR entryPoint,
	LPCWSTR targetProfile,
	std::vector<LPCWSTR> arguments)
{
	DXASSERT(shaderAbsolutePath || entryPoint || targetProfile, "Shader path, entry point or target profile has been not provided");

	// Create blob from shader file
	uint32_t codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ThrowIfFailed(library->CreateBlobFromFile(shaderAbsolutePath, &codePage, sourceBlob.GetAddressOf()));

	// Get amount of shader defines
	constexpr auto shaderDefinesAmount = static_cast<UINT32>(sizeof(DxcDefine) / sizeof(shaderDefines[0]));

	// Source buffer of shader
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr		= sourceBlob->GetBufferPointer();
	sourceBuffer.Size		= sourceBlob->GetBufferSize();
	sourceBuffer.Encoding	= codePage;

	// Use if IDxcCompiler3 TODO
	ComPtr<IDxcCompilerArgs> CompilerArgs;
	utils->BuildArguments(
		shaderAbsolutePath,																		// pSourceName
		entryPoint,																				// pEntryPoint
		targetProfile,																			// pTargetProfile, currently it has to be lower-case
		arguments.data(), arguments.size(),														// pArguments, argCount
		shaderDefines ? &shaderDefines[0] : nullptr, shaderDefines ? shaderDefinesAmount : 0,	// pDefines, defineCount
		CompilerArgs.GetAddressOf()																// out ppArgs
	);

	// Compile Shader
	ComPtr<IDxcResult> result{};
	auto hr = compiler->Compile(
		&sourceBuffer,																		// pSource																								
		CompilerArgs->GetArguments(), CompilerArgs->GetCount(),								// pArguments, argCount
		dxcIncludeHandler.Get(),															// pIncludeHandler
		IID_PPV_ARGS(result.GetAddressOf())													// ppResult
	);

	if (SUCCEEDED(hr))
	{
		result->GetStatus(&hr);
	}

	if (FAILED(hr))
	{
		ComPtr<IDxcBlobUtf8> errorsBlob;
		hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorsBlob), nullptr);
		if (errorsBlob && errorsBlob->GetStringLength())
		{
			const auto errorsBlobMessage = static_cast<const char*>(errorsBlob->GetBufferPointer());
			DXLOG("HRESULT of %08d | %s \n", static_cast<HRESULT>(hr), errorsBlobMessage);
		}
		//TODO Handle compilation error somehow instead of nullptr... 
		return nullptr;
	}

	ComPtr<ID3DBlob> code;
	result->GetResult(reinterpret_cast<IDxcBlob**>(code.GetAddressOf())); //cast ID3DBlob to IDxcBlob

	return code;
}

// Old D3D HLSL Shader compiler
ComPtr<ID3DBlob> D3D12ShaderCompiler::CompileShaderD3D(
	LPCWSTR shaderAbsolutePath, 
	D3D_SHADER_MACRO const * shaderDefines,
	LPCSTR entryPoint, 
	LPCSTR targetProfile)
{
	DXASSERT(shaderAbsolutePath || entryPoint || targetProfile, "Shader path, entry point or target profile has been not provided");

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
		shaderAbsolutePath, 
		shaderDefines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, 
		targetProfile,
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