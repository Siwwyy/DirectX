
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"


#include <wrl.h>


// Own includes
#include "D3D12App.h"
#include "Win32Proc.h"
////


_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    D3D12App dx12App(1280u, 720u, L"MyApp");
    return Win32Proc::Run(hInstance, nCmdShow, &dx12App);
}