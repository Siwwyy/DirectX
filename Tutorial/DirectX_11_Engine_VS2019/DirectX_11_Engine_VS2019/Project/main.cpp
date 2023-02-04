#include "Engine.h"

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	//constexpr HRESULT hr = S_OK;
	//if (SUCCEEDED(hr))
	//{
	//	MessageBoxA(nullptr, "SUCCESS", "SUCCESS", NULL);
	//}
	//if (FAILED(hr))
	//{
	//	ErrorLogger::Log(E_INVALIDARG, "FAILURE");
	//}


	//RenderWindow rw;
	//rw.Initialize(hInstance, "Title", "MyWindowClass", 800, 600);
	//while (rw.ProcessMessages())
	//{
	//	//Sleep(50);
	//}


	Engine engine;
	engine.Initialize(hInstance, "Title", "MyWindowClass", 800, 600);
	while (engine.ProcessMessages())
	{
		engine.Update();
		engine.RenderFrame();
		//Sleep(50);
	}

	return 0;
}