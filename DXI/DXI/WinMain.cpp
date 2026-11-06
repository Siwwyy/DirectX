
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

// PER APP DEFINES
//#define DEBUG_MODE 1

// Own includes
#include "Win32Proc.h"
#include "CommandLine.h"
////


_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    // First, so everything downstream can read a switch. It takes the command line from
    // the process rather than from lpCmdLine, which is ANSI and already split badly.
    DXICMD::Parse();

    if (DXICMD::ShowHelpDocs())
    {
        DXICMD::LogUsage();
    }

    // Which app actually runs is the CurrentApp alias of Win32Proc.h
    CurrentApp dx12App(static_cast<UINT>(DXICMD::GetInt(L"width", 1280)),
                       static_cast<UINT>(DXICMD::GetInt(L"height", 720)),
                       L"MyApp");

    return Win32Proc::Run(hInstance, nCmdShow, &dx12App);
}