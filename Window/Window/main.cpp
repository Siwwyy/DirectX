//// HelloWindowsDesktop.cpp
//// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c
//
//#include <Windows.h>
//#include <tchar.h>
//
//
//#include <windows.h>
//
//#include <dxgi1_6.h>
//#include <d3d12.h>
//#include <d2d1_3.h>
//#include <dwrite.h>
//#include <d3d11on12.h>
//#include <DirectXMath.h>
//
//#include <wrl.h>
//#include <string>
//#include <vector>
//#include <memory>
//#include <shellapi.h>
//
//
//
//// Global variables
//
//// The main window class name.
//static TCHAR szWindowClass[] = _T("DesktopApp");
//
//// The string that appears in the application's title bar.
//static TCHAR szTitle[] = _T("Windows Desktop Guided Tour Application");
//
//// Stored instance handle for use in Win32 API calls such as FindResource
//HINSTANCE hInst;
//
//// Forward declarations of functions included in this code module:
//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//
//int WINAPI WinMain(
//	_In_ HINSTANCE hInstance,
//	_In_opt_ HINSTANCE hPrevInstance,
//	_In_ LPSTR     lpCmdLine,
//	_In_ int       nCmdShow
//)
//{
//	WNDCLASSEX wcex;
//
//	wcex.cbSize = sizeof(WNDCLASSEX);
//	wcex.style = CS_HREDRAW | CS_VREDRAW;
//	wcex.lpfnWndProc = WndProc;
//	wcex.cbClsExtra = 0;
//	wcex.cbWndExtra = 0;
//	wcex.hInstance = hInstance;
//	wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
//	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//	wcex.lpszMenuName = nullptr;
//	wcex.lpszClassName = szWindowClass;
//	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
//
//	if (!RegisterClassEx(&wcex))
//	{
//		MessageBox(nullptr,
//			_T("Call to RegisterClassEx failed!"),
//			_T("Windows Desktop Guided Tour"),
//			NULL);
//
//		return 1;
//	}
//
//	// Store instance handle in our global variable
//	hInst = hInstance;
//
//	// The parameters to CreateWindowEx explained:
//	// WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
//	// szWindowClass: the name of the application
//	// szTitle: the text that appears in the title bar
//	// WS_OVERLAPPEDWINDOW: the type of window to create
//	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
//	// 500, 100: initial size (width, length)
//	// NULL: the parent of this window
//	// NULL: this application does not have a menu bar
//	// hInstance: the first parameter from WinMain
//	// NULL: not used in this application
//	HWND hWnd = CreateWindowEx(
//		WS_EX_OVERLAPPEDWINDOW,
//		szWindowClass,
//		szTitle,
//		WS_OVERLAPPEDWINDOW,
//		CW_USEDEFAULT, CW_USEDEFAULT,
//		500, 100,
//		nullptr,
//		nullptr,
//		hInstance,
//		nullptr
//	);
//
//	if (!hWnd)
//	{
//		MessageBox(nullptr,
//			_T("Call to CreateWindow failed!"),
//			_T("Windows Desktop Guided Tour"),
//			NULL);
//
//		return 1;
//	}
//
//	// The parameters to ShowWindow explained:
//	// hWnd: the value returned from CreateWindow
//	// nCmdShow: the fourth parameter from WinMain
//	ShowWindow(hWnd,
//		nCmdShow);
//	UpdateWindow(hWnd);
//
//	//// Main message loop:
//	//MSG msg;
//	//while (GetMessage(&msg, NULL, 0, 0))
//	//{
//	//	TranslateMessage(&msg);
//	//	DispatchMessage(&msg);
//	//}
//
//	bool bGotMsg = false;
//	MSG  msg;
//	msg.message = WM_NULL;
//	PeekMessage(&msg, nullptr, 0U, 0U, PM_NOREMOVE);
//
//	while (WM_QUIT != msg.message)
//	{
//		// Process window events.
//		// Use PeekMessage() so we can use idle time to render the scene. 
//		bGotMsg = (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE) != 0);
//
//		if (bGotMsg)
//		{
//			// Translate and dispatch the message
//			TranslateMessage(&msg);
//			DispatchMessage(&msg);
//		}
//		else
//		{
//			//// Update the scene.
//			//renderer->Update();
//
//			//// Render frames during idle time (when no messages are waiting).
//			//renderer->Render();
//
//			//// Present the frame to the screen.
//			//deviceResources->Present();
//		}
//	}
//
//
//	D3D_FEATURE_LEVEL levels[] = {
//	D3D_FEATURE_LEVEL_9_1,
//	D3D_FEATURE_LEVEL_9_2,
//	D3D_FEATURE_LEVEL_9_3,
//	D3D_FEATURE_LEVEL_10_0,
//	D3D_FEATURE_LEVEL_10_1,
//	D3D_FEATURE_LEVEL_11_0,
//	D3D_FEATURE_LEVEL_11_1
//	};
//
//
//	// This flag adds support for surfaces with a color-channel ordering different
//// from the API default. It is required for compatibility with Direct2D.
//	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
//
//#if defined(DEBUG) || defined(_DEBUG)
//	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif
//
//	// Create the Direct3D 11 API device object and a corresponding context.
//	Microsoft::WRL::ComPtr<ID3D11Device>        device;
//	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
//
//	D3D_FEATURE_LEVEL m_featureLevel;
//	HRESULT hr = D3D11CreateDevice(
//		nullptr,                    // Specify nullptr to use the default adapter.
//		D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
//		0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
//		deviceFlags,                // Set debug and Direct2D compatibility flags.
//		levels,                     // List of feature levels this app can support.
//		ARRAYSIZE(levels),          // Size of the list above.
//		D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
//		&device,                    // Returns the Direct3D device created.
//		&m_featureLevel,            // Returns feature level of device created.
//		&context                    // Returns the device immediate context.
//	);
//
//	if (FAILED(hr))
//	{
//		// Handle device interface creation failure if it occurs.
//		// For example, reduce the feature level requirement, or fail over 
//		// to WARP rendering.
//	}
//
//	// Store pointers to the Direct3D 11.1 API device and immediate context.
//	device.As(&m_pd3dDevice);
//	context.As(&m_pd3dDeviceContext);
//
//	return static_cast<int>(msg.wParam);
//}
//
//
////  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
////
////  PURPOSE:  Processes messages for the main window.
////
////  WM_PAINT    - Paint the main window
////  WM_DESTROY  - post a quit message and return
//LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	//PAINTSTRUCT ps;
//	//HDC hdc;
//	//TCHAR greeting[] = _T("Hello, Windows desktop!");
//
//	//switch (message)
//	//{
//	//case WM_PAINT:
//	//    hdc = BeginPaint(hWnd, &ps);
//
//	//    // Here your application is laid out.
//	//    // For this introduction, we just print out "Hello, Windows desktop!"
//	//    // in the top left corner.
//	//    TextOut(hdc,
//	//        5, 5,
//	//        greeting, _tcslen(greeting));
//	//    // End application-specific layout section.
//
//	//    EndPaint(hWnd, &ps);
//	//    break;
//	//case WM_DESTROY:
//	//    PostQuitMessage(0);
//	//    break;
//	//default:
//	//    return DefWindowProc(hWnd, message, wParam, lParam);
//	//    break;
//	//}
//
//	//return 0;
//
//	switch (message)
//	{
//	case WM_CLOSE:
//	{
//		HMENU hMenu;
//		hMenu = GetMenu(hWnd);
//		if (hMenu != NULL)
//		{
//			DestroyMenu(hMenu);
//		}
//		DestroyWindow(hWnd);
//		UnregisterClass(
//			szTitle,
//			hInst
//		);
//		return 0;
//	}
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		break;
//	default:
//		return DefWindowProc(hWnd, message, wParam, lParam);
//	}
//
//	return 0;
//}