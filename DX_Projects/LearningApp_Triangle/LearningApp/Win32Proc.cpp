//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "Win32Proc.h"

#include <cstdio>

HWND Win32Proc::hwnd = nullptr;


int Win32Proc::Run(HINSTANCE hInstance, int nCmdShow, D3D12App * dx12App)
{
	// Parse Command line arguments
	//TODO Parsing command line arguments

    //Constants
    const wchar_t*      windowTitle          = dx12App->GetWindowName().c_str();
    const LONG          MaxWidth             = dx12App->GetWindowWidth();
    const LONG          MaxHeight            = dx12App->GetWindowHeight();

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
    dx12App->OnInit();

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
    dx12App->OnDestroy();

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
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
    return 0;

    case WM_KEYDOWN:
    {
        //PAINTSTRUCT ps;
        //HDC hdc = BeginPaint(hwnd, &ps);

        //// All painting occurs here, between BeginPaint and EndPaint.
        ////FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        //FillRect(hdc, &ps.rcPaint, (HBRUSH)COLOR_HIGHLIGHT);
        //EndPaint(hwnd, &ps);

    }
    return 0;

    case WM_KEYUP:
    {
        //OutputDebugStringA(reinterpret_cast<LPCSTR>(wParam));

        //LPCSTR aaa = static_cast<LPCSTR>(wParam);
        //OutputDebugStringA(static_cast<LPCSTR>(wParam));
        //OutputDebugStringA("\n");
        ////OutputDebugStringA(reinterpret_cast<LPCSTR>(lParam));
        //OutputDebugStringA("\n");
        //OutputDebugStringA("======================================");
    }
    return 0;

    case WM_MOUSEMOVE:
    {

    }


    case WM_PAINT:
    {
        dx12App->OnUpdate();
        dx12App->OnRender();
    }
    return 0;

    case WM_DESTROY:
    {
		PostQuitMessage(0);
    }
    return 0;

	default: ;
    }
    // Handle any messages.
    return DefWindowProc(hWnd, message, wParam, lParam);
}
