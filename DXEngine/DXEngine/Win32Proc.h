
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once
#include <D3Dcompiler.h>

#include "D3D12App.h"
#include "DXApp3D.h"

//using CurrentApp = DXApp3D;
using CurrentApp = D3D12App;

class Win32Proc
{
public:

    static int Run(HINSTANCE hInstance, int nCmdShow, CurrentApp * dx12App);
    static HWND GetHwnd() { return hwnd; }

protected:

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

    static HWND hwnd;
};