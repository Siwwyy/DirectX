#include "LightPass.h"	


LightPass::LightPass()
{
	// Init shaders
	constexpr const wchar_t* VertexShaderPath = L"shaders//LightPass//vertex_shader.hlsl";
	constexpr const wchar_t* PixelShaderPath = L"shaders//LightPass//pixel_shader.hlsl";

	std::vector<LPCWSTR> arguments;
	// String arguments
	//arguments.push_back(L"-enable-16bit-types");
	//arguments.push_back(L"Qstrip_reflect");
	arguments.push_back(L"-Werror");
	arguments.push_back(L"-Wconversion");
	// Defines arguments
	arguments.push_back(DXC_ARG_ALL_RESOURCES_BOUND);
#if DEBUG_MODE
	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);	//-Od
	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);	//-WX
	arguments.push_back(DXC_ARG_DEBUG);					//-Zi
#else
	arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);	//
#endif

	PSO.VertexShader	= DXIShaderCompiler.CompileShader(VertexShaderPath, L"main", L"vs_6_0", arguments);
	PSO.PixelShader		= DXIShaderCompiler.CompileShader(PixelShaderPath, L"main", L"ps_6_0", arguments);
}

HRESULT LightPass::Init(DXDevice * Device, DXGraphicsCommandList * CommandList) noexcept
{
	return E_NOTIMPL;
}

void LightPass::Render(DXGraphicsCommandList * const CommandList, const Primitive& Primitive) noexcept
{

}