#include "D3D12ShaderCompiler.h"

using namespace Helpers;

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
}

ComPtr<ID3DBlob> D3D12ShaderCompiler::CompileShader(
	LPCWSTR shaderAbsolutePath,
	const DxcDefine* shaderDefines,
	LPCWSTR entryPoint,
	LPCWSTR targetProfile)
{
	// Create blob from shader file
	uint32_t codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ThrowIfFailed(library->CreateBlobFromFile(shaderAbsolutePath, &codePage, &sourceBlob));

	// Compile Shader
	constexpr auto shaderDefinesAmount = static_cast<UINT32>(sizeof(DxcDefine) / sizeof(shaderDefines[0]));
	ComPtr<IDxcOperationResult> result;
	auto hr = compiler->Compile(
		sourceBlob.Get(),							// pSource
		shaderAbsolutePath,							// pSourceName
		entryPoint,									// pEntryPoint
	    targetProfile,								// pTargetProfile, currently it has to be lower-case
		nullptr, 0,									// pArguments, argCount
		shaderDefines ? &shaderDefines[0] : nullptr, shaderDefines ? shaderDefinesAmount : 0,		// pDefines, defineCount
	    nullptr,									// pIncludeHandler
		&result);									// ppResult

	if (SUCCEEDED(hr))
	{
		result->GetStatus(&hr);
	}

	if (FAILED(hr))
	{
		if (result)
		{
			ComPtr<IDxcBlobEncoding> errorsBlob;
			hr = result->GetErrorBuffer(&errorsBlob);
			if (SUCCEEDED(hr) && errorsBlob)
			{
				OutputDebugStringA(static_cast<const char*>(errorsBlob->GetBufferPointer()));
			}
		}
		// Handle compilation error somehow...
	}
	ComPtr<ID3DBlob> code;
	result->GetResult(reinterpret_cast<IDxcBlob**>(code.GetAddressOf())); //cast ID3DBlob to IDxcBlob

	return code;
}