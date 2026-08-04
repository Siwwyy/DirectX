//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Win32Proc.h"

HWND Win32Proc::hwnd = nullptr;

int Win32Proc::Run(HINSTANCE hInstance, int nCmdShow, CurrentApp * dx12App)
{
	// Parse Command line arguments
	//TODO Parsing command line arguments

	//Constants
	const std::wstring	windowTitleStr	= dx12App->GetWindowName();
	const LONG          MaxWidth		= dx12App->GetWindowWidth();
	const LONG          MaxHeight		= dx12App->GetWindowHeight();

	// Initialization of window class
	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc; //pointer to WindowProcess function which returns LRESULT
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = L"D3D12SampleClass";
	RegisterClassEx(&windowClass); //WNDCLASSEX registration

	// Create Window Rect
	RECT windowRect = { 0, 0, MaxWidth, MaxHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	hwnd = CreateWindow(
		windowClass.lpszClassName,
		windowTitleStr.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,        // We have no parent window.
		nullptr,        // We aren't using menus.
		hInstance,
		dx12App);

	//TODO Add initialization of DX12 environment
	dx12App->Initialize();

	ShowWindow(hwnd, nCmdShow);

	// Main sample loop.
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//TODO Destroy DX12 environment here
	dx12App->Destroy();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

LRESULT Win32Proc::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CurrentApp* dx12App = reinterpret_cast<CurrentApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	//DXASSERT(dx12App == nullptr, "Can not retrieve app pointer");

	// create an instance of timer
	//Helpers::Timer timer;
	switch (message)
	{
	case WM_CREATE:
	{
		// Save the D3D12App* passed in to CreateWindow.
		const auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	return 0;


	// WM_KEYDOWN
	case WM_KEYDOWN:
	{
		dx12App->OnKeyDown(static_cast<UINT8>(wParam));
	}
	return 0;

	//WM KEYUP
	case WM_KEYUP:
	{
		dx12App->OnKeyUp(static_cast<UINT8>(wParam));
	}

	return 0;

	case WM_MOUSEMOVE:
	{

	}


	case WM_PAINT:
	{
		//auto DeltaTime = timer.GetFrameDelta();
		dx12App->Update(static_cast<float>(0.0)); //or DeltaTime
		dx12App->Render();
	}
	return 0;

	case WM_CLOSE:
	case WM_DESTROY:
	{
		dx12App->Destroy();
		PostQuitMessage(0);
	}
	return 0;

	default:;
	}
	// Handle any messages.
	return DefWindowProc(hWnd, message, wParam, lParam);
}