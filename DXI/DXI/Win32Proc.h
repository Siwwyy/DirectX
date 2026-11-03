
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef WIN32_PROC_H_INCLUDED
#define WIN32_PROC_H_INCLUDED

#include "D3D12App.h"
#include "D3D12Renderer.h"
//#include "DXApp3D.h"
//#include "D3D123DApp.h"

//using CurrentApp = DXApp3D;
//using CurrentApp = D3D123DApp;
//using CurrentApp = D3D12App;
using CurrentApp = D3D12Renderer;

class Win32Proc
{
public:

    static int Run(HINSTANCE hInstance, int nCmdShow, CurrentApp * dx12App);
    static HWND GetHwnd() { return hwnd; }

protected:

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

    static HWND hwnd;

    // Mouse look | Rotation only happens while the right button is held, so moving the
    // cursor over the window does not spin the camera around
    static POINT    LastMousePos;
    static bool     bIsRotating;
};

#endif // WIN32_PROC_H_INCLUDED