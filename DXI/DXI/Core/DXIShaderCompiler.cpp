#include "DXIShaderCompiler.h"

#include <cassert>
#include <D3Dcompiler.h>

using namespace Helpers;

DXIShaderCompiler::DXIShaderCompiler()
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary,	IID_PPV_ARGS(&Library)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler,	IID_PPV_ARGS(&Compiler)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils,		IID_PPV_ARGS(&Utils)));
	ThrowIfFailed(Library->CreateIncludeHandler(&DxcIncludeHandler));
}

namespace
{
	// The compiler speaks utf8, the message box wants wide
	std::wstring Utf8ToWide(const std::string& Utf8)
	{
		if (Utf8.empty())
		{
			return {};
		}

		const int WideLength = MultiByteToWideChar(CP_UTF8, 0, Utf8.data(), static_cast<int>(Utf8.size()), nullptr, 0);
		std::wstring Wide(static_cast<size_t>(WideLength), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, Utf8.data(), static_cast<int>(Utf8.size()), Wide.data(), WideLength);
		return Wide;
	}

	// A message box shows a screenful at best, and a build with -Werror can turn a whole file of
	// warnings into errors. The start is where the first and usually the real one is.
	constexpr size_t MESSAGE_BOX_MAX_OUTPUT_CHARS = 3000;
}

void DXIShaderCompiler::ReportCompileFailure(
	LPCWSTR ShaderAbsolutePath,
	LPCWSTR EntryPoint,
	LPCWSTR TargetProfile,
	HRESULT HrResult,
	const std::string& CompilerOutput)
{
	// Only the HRESULT goes through a fixed buffer, it is always ten characters. The rest are
	// strings of any length, a long path included, so they are joined rather than formatted.
	wchar_t HrText[16] = {};
	swprintf_s(HrText, L"0x%08X", static_cast<UINT>(HrResult));

	const std::wstring Header = std::wstring(L"Shader compilation failed\n\n")
		+ L"File:\t\t"			+ (ShaderAbsolutePath ? ShaderAbsolutePath : L"(none)") + L"\n"
		+ L"Entry point:\t"		+ (EntryPoint ? EntryPoint : L"(none)") + L"\n"
		+ L"Target:\t\t"			+ (TargetProfile ? TargetProfile : L"(none)") + L"\n"
		+ L"HRESULT:\t"			+ HrText;

	const std::wstring Output = CompilerOutput.empty()
		? std::wstring(L"The compiler gave no error text. Check the arguments and the target profile.")
		: Utf8ToWide(CompilerOutput);

	// Everything, to the debugger. Straight out of a string rather than through DXLOG, which
	// formats into a fixed buffer and aborts on a message that does not fit it.
	const std::wstring FullMessage = Header + L"\n\n" + Output + L"\n";
	OutputDebugStringW(FullMessage.c_str());

	// A screenful to the user
	std::wstring BoxOutput = Output;
	if (BoxOutput.size() > MESSAGE_BOX_MAX_OUTPUT_CHARS)
	{
		BoxOutput.resize(MESSAGE_BOX_MAX_OUTPUT_CHARS);
		BoxOutput += L"\n\n... shortened, the whole output is in the debugger output window";
	}

	const std::wstring BoxMessage = Header + L"\n\n" + BoxOutput;
	MessageBoxW(nullptr, BoxMessage.c_str(), L"DXI | Shader compilation error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);

	// The box is the explanation, this only has to stop the app at the failure
	throw std::runtime_error("Shader compilation failed, see the message box and the debugger output");
}

// Old D3D HLSL Shader compiler
ComPtr<ID3DBlob> DXIShaderCompiler::CompileShaderD3D(
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


// make it always last...
DXIShaderCompiler GShaderCompiler = {};
