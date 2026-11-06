
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef WIN32_PROC_H_INCLUDED
#define WIN32_PROC_H_INCLUDED

#include "D3D12Renderer.h"

// Which app Run() drives. Swapping this is how a new renderer gets taken for a spin
// without touching WinMain; it only has to offer the same Initialize/Update/Render surface.
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

    // Resizing | A drag reports a new size on every mouse move. Rebuilding the render
    // targets that often would wait for the GPU dozens of times, so the size is held here
    // and applied when the drag ends. See WM_SIZE.
    static bool     bIsResizing;
    static UINT     PendingWidth;
    static UINT     PendingHeight;
};

#endif // WIN32_PROC_H_INCLUDED