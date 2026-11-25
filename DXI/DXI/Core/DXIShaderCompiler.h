
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef DXI_SHADER_COMPILER_H_INCLUDED
#define DXI_SHADER_COMPILER_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include <dxcapi.h> //for new DX shader compiler API | Has to be included at the end, otherwise got many errors

template<SIZE_T DefinesCount>
struct DXShaderDefines
{
	// Friendship here, just in case
	friend class DXIShaderCompiler;

public:

	// Ctor & Dctor
	~DXShaderDefines() = default;
	DXShaderDefines()
	{
		// Default init
		for (UINT i = 0; i < DefinesCount; ++i)
		{
			Defines[i] = EmptyDxcDefine;
		}
	}

	/**
	 * Add Define to the array of defines
	 */
	FORCEINLINE void AddDefine(DxcDefine ShaderDefine);

private:

	// Array of actual defines
	DxcDefine Defines[DefinesCount + 1];

	// Current index of the define to be added
	SIZE_T CurrentIndex = 0;

	// Special registered sign of empty define
	const DxcDefine EmptyDxcDefine = { L"EMPTYDEFINE", L"0" };
};

template<SIZE_T DefinesCount>
inline void DXShaderDefines<DefinesCount>::AddDefine(DxcDefine ShaderDefine)
{
	DXASSERT(CurrentIndex < DefinesCount, "Cannot add more defines than the specified DefinesCount");
	DXASSERT(std::wstring(EmptyDxcDefine.Name) != std::wstring(ShaderDefine.Name), "Cannot add define with the same empty dxc name!");
	Defines[CurrentIndex] = ShaderDefine;
	++CurrentIndex;
}


// D3D12 Shader Compiler class using old D3DCompile API and new DXC API
class DXIShaderCompiler
{
public:
	DISABLE_COPY_MOVE(DXIShaderCompiler)

	DXIShaderCompiler();
	~DXIShaderCompiler() = default;

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

	/**
	 * Reports a shader that failed to compile and never returns.
	 *
	 * A message box with the file, the entry point, the target and everything the compiler said,
	 * so the cause is in front of you instead of in a debugger output window nobody has open. The
	 * whole text also goes to the debugger output, which a message box has to shorten.
	 *
	 * Then it throws. A pass cannot build a pipeline out of a shader that does not exist, and a
	 * null blob handed on would only fail later, in an assert in PSO creation that names neither
	 * the file nor the error.
	 *
	 * @param CompilerOutput The compiler's own error text, utf8. Empty when it failed without saying why.
	 */
	[[noreturn]] static void ReportCompileFailure(
		LPCWSTR ShaderAbsolutePath,
		LPCWSTR EntryPoint,
		LPCWSTR TargetProfile,
		HRESULT HrResult,
		const std::string& CompilerOutput);

	// IDxc variables
	ComPtr<IDxcLibrary>							Library;
	ComPtr<IDxcCompiler3>						Compiler;	//Use IDxcCompiler3 instead of IDxcCompiler (because of Deprecatness Compile() function)
	ComPtr<IDxcUtils>							Utils;
	ComPtr<IDxcIncludeHandler>					DxcIncludeHandler;

};

// Compile shader DXI
template<SIZE_T DefinesCount>
FORCEINLINE ComPtr<ID3DBlob> DXIShaderCompiler::CompileShader(
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
		// What the compiler said. The HRESULT of the failure is kept as it is, getting the output
		// is a call of its own and its result must not replace the reason we are here.
		// The result can be missing altogether when Compile itself failed rather than the shader.
		std::string CompilerOutput;
		if (result)
		{
			ComPtr<IDxcBlobUtf8> errorsBlob;
			result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorsBlob), nullptr);
			if (errorsBlob && errorsBlob->GetStringLength())
			{
				CompilerOutput.assign(errorsBlob->GetStringPointer(), errorsBlob->GetStringLength());
			}
		}

		ReportCompileFailure(ShaderAbsolutePath, EntryPoint, TargetProfile, HrResult, CompilerOutput);
	}

	ComPtr<ID3DBlob> code;
	result->GetResult(reinterpret_cast<IDxcBlob**>(code.GetAddressOf())); //cast ID3DBlob to IDxcBlob

	return code;
}


// extern
extern DXIShaderCompiler GShaderCompiler;

#endif // DXI_SHADER_COMPILER_H_INCLUDED
