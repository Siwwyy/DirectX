//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Win32Proc.h"
#include "D3D12App.h"

HWND Win32Proc::hwnd = nullptr;


int Win32Proc::Run(HINSTANCE hInstance, int nCmdShow, D3D12App * dx12App)
{
	// Parse Command line arguments
	//TODO Parsing command line arguments

	//Constants
	const wchar_t*      windowTitle = dx12App->GetWindowName().c_str();
	const LONG          MaxWidth = dx12App->GetWindowWidth();
	const LONG          MaxHeight = dx12App->GetWindowHeight();

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
		windowTitle,
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
	D3D12App* dx12App = reinterpret_cast<D3D12App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

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
		switch (wParam)
		{
		case VK_UP:
		{
			// Process the UP ARROW key.
			dx12App->ArrowUp();
		}
		return 0;

		case VK_DOWN:
		{
			// Process the UP ARROW key.
			dx12App->ArrowDown();
		}
		return 0;
		default: return 0;
		}
	}
	return 0;
	// !WM_KEYDOWN
	
	case WM_KEYUP:
	{

	}
	return 0;

	case WM_MOUSEMOVE:
	{

	}


	case WM_PAINT:
	{
		dx12App->Update();
		dx12App->Render();
	}
	return 0;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	return 0;

	default:;
	}
	// Handle any messages.
	return DefWindowProc(hWnd, message, wParam, lParam);
	}
