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

ComPtr<ID3DBlob> D3D12ShaderCompiler::CompileShader(
	LPCWSTR ShaderAbsolutePath,
	DxcDefine const * const ShaderDefines,
	LPCWSTR EntryPoint,
	LPCWSTR TargetProfile,
	std::vector<LPCWSTR> Arguments)
{
	DXASSERT(ShaderAbsolutePath || EntryPoint || TargetProfile, "Shader path, entry point or target profile has been not provided");

	// Create blob from shader file
	uint32_t codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> SourceBlob;
	ThrowIfFailed(Library->CreateBlobFromFile(ShaderAbsolutePath, &codePage, SourceBlob.GetAddressOf()));

	// Get amount of shader defines
	constexpr auto ShaderDefinesAmount = static_cast<UINT32>(sizeof(DxcDefine) / sizeof(ShaderDefines[0]));

	// Source buffer of shader
	DxcBuffer SourceBuffer;
	SourceBuffer.Ptr		= SourceBlob->GetBufferPointer();
	SourceBuffer.Size		= SourceBlob->GetBufferSize();
	SourceBuffer.Encoding	= codePage;

	// Use if IDxcCompiler3 TODO
	ComPtr<IDxcCompilerArgs> CompilerArgs;
	auto HrResult = Utils->BuildArguments(
		ShaderAbsolutePath,																		// pSourceName
		EntryPoint,																				// pEntryPoint
		TargetProfile,																			// pTargetProfile, currently it has to be lower-case
		Arguments.data(), Arguments.size(),														// pArguments, argCount
		ShaderDefines ? &ShaderDefines[0] : nullptr, ShaderDefines ? ShaderDefinesAmount : 0,	// pDefines, defineCount
		CompilerArgs.GetAddressOf()																// out ppArgs
	);

	// Compile Shader
	ComPtr<IDxcResult> result{};
	HrResult = Compiler->Compile(
		&SourceBuffer,																		// pSource																								
		CompilerArgs->GetArguments(), CompilerArgs->GetCount(),								// pArguments, argCount
		DxcIncludeHandler.Get(),															// pIncludeHandler
		IID_PPV_ARGS(result.GetAddressOf())													// ppResult
	);

	if (SUCCEEDED(HrResult))
	{
		result->GetStatus(&HrResult);
	}

	if (FAILED(HrResult))
	{
		ComPtr<IDxcBlobUtf8> errorsBlob;
		HrResult = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorsBlob), nullptr);
		if (errorsBlob && errorsBlob->GetStringLength())
		{
			const auto errorsBlobMessage = static_cast<const char*>(errorsBlob->GetBufferPointer());
			DXLOG("HRESULT of %08d | %s \n", static_cast<HRESULT>(HrResult), errorsBlobMessage);
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