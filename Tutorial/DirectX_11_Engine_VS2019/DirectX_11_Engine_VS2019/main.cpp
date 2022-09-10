#include "ErrorLogger.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DirectXTK.lib")

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	constexpr HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		MessageBoxA(nullptr, "SUCCESS", "SUCCESS", NULL);
	}
	if (FAILED(hr))
	{
		ErrorLogger::Log(E_INVALIDARG, "FAILURE");
	}
	return 0;
}