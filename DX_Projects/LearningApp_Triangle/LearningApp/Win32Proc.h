
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#pragma once
#include <D3Dcompiler.h>

#include "D3D12App.h"


class Win32Proc
{
public:

    static int Run(HINSTANCE hInstance, int nCmdShow, D3D12App * dx12App);
    static HWND GetHwnd() { return hwnd; }

protected:

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND hwnd;
};
