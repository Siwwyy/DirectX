
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_SHADER_COMPILER_H_INCLUDED
#define D3D12_SHADER_COMPILER_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include <dxcapi.h> //for new DX shader compiler API | Has to be included at the end, otherwise got many errors

template<SIZE_T DefinesCount>
struct DXShaderDefines
{
	// Friendship here, just in case
	friend class D3D12ShaderCompiler;

public:

	// Add Define to the array of defines
	FORCEINLINE void AddDefine(DxcDefine ShaderDefine);

private:

	// Array of actual defines
	DxcDefine Defines[DefinesCount + 1];

	// Current index of the define to be added
	SIZE_T CurrentIndex = 0;
};

template<SIZE_T DefinesCount>
inline void DXShaderDefines<DefinesCount>::AddDefine(DxcDefine ShaderDefine)
{
	DXASSERT(CurrentIndex < DefinesCount, "Cannot add more defines than the specified DefinesCount");
	Defines[CurrentIndex] = ShaderDefine;
	++CurrentIndex;
}


// D3D12 Shader Compiler class using old D3DCompile API and new DXC API
class D3D12ShaderCompiler
{
public:
	DISABLE_COPY_MOVE(D3D12ShaderCompiler)

	D3D12ShaderCompiler();
	~D3D12ShaderCompiler() = default;

	template<SIZE_T DefinesCount = 0>
	ComPtr<ID3DBlob> CompileShader(
		LPCWSTR ShaderAbsolutePath,
		LPCWSTR EntryPoint,
		LPCWSTR TargetProfile,
		std::vector<LPCWSTR> Arguments = {},
		DXShaderDefines<DefinesCount> const * const ShaderDefines = nullptr);	//TODO Add compile arguments for compilation process, See https://simoncoenen.com/blog/programming/graphics/DxcCompiling

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

// Compile shader DXI
template<SIZE_T DefinesCount>
FORCEINLINE ComPtr<ID3DBlob> D3D12ShaderCompiler::CompileShader(
	LPCWSTR ShaderAbsolutePath, 
	LPCWSTR EntryPoint, 
	LPCWSTR TargetProfile, 
	std::vector<LPCWSTR> Arguments, 
	DXShaderDefines<DefinesCount> const * const ShaderDefines)
{
	DXASSERT(ShaderAbsolutePath || EntryPoint || TargetProfile, "Shader path, entry point or target profile has been not provided");

	// Create blob from shader file
	uint32_t codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> SourceBlob;
	Helpers::ThrowIfFailed(Library->CreateBlobFromFile(ShaderAbsolutePath, &codePage, SourceBlob.GetAddressOf()));

	// Source buffer of shader
	DxcBuffer SourceBuffer;
	SourceBuffer.Ptr = SourceBlob->GetBufferPointer();
	SourceBuffer.Size = SourceBlob->GetBufferSize();
	SourceBuffer.Encoding = codePage;

	// Use if IDxcCompiler3 TODO
	ComPtr<IDxcCompilerArgs> CompilerArgs;
	auto HrResult = Utils->BuildArguments(
		ShaderAbsolutePath,																		// pSourceName
		EntryPoint,																				// pEntryPoint
		TargetProfile,																			// pTargetProfile, currently it has to be lower-case
		Arguments.data(), Arguments.size(),														// pArguments, argCount
		ShaderDefines ? &ShaderDefines->Defines[0] : nullptr, ShaderDefines ? DefinesCount : 0,	// pDefines, defineCount | Just in case check for Defines, even though we have default nullptr for ShaderDefines
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


// extern
extern D3D12ShaderCompiler DXIShaderCompiler;

#endif // D3D12_SHADER_COMPILER_H_INCLUDED