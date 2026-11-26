//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXI_COMMANDLINE_H_INCLUDED
#define DXI_COMMANDLINE_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"

#include <string>

// Switches read off the command line at startup.
//
//     DXIApp.exe -gpuvalidation -warp -width 1920 -height 1080
//
// Flags accept -name, --name or /name and are matched case insensitively. A value may be
// given either as "-width 1920" or "-width=1920".
//
// Parse() takes nothing: it reads the process command line itself through GetCommandLineW,
// so nothing has to be threaded down from WinMain.
//
// A host that is not the executable, a Python process loading the engine as a module, has a
// command line that says nothing about the engine. It hands the switches over as a string
// instead, written the same way: L"-warp -nobreak". Index 0 is the program name in a real
// command line, so the string starts with one too, which can be anything.
namespace DXICMD
{
	/**
	 * Call once, before anything reads a switch
	 */
	void Parse();

	/**
	 * Same, from a string instead of the process command line. The first call of either wins.
	 */
	void Parse(const wchar_t* CommandLine);

	/**
	 * True when the switch is present at all, with or without a value
	 */
	_NODISCARD bool HasFlag(const wchar_t* Name);

	/**
	 * Value of a switch, or Fallback when it is absent or not a number
	 */
	_NODISCARD int GetInt(const wchar_t* Name, const int Fallback);

	/**
	 * Value of a switch as written, or Fallback when it is absent or carries no value.
	 * Case is preserved, unlike the switch name, so compare it with _wcsicmp.
	 */
	_NODISCARD std::wstring GetString(const wchar_t* Name, const wchar_t* Fallback);

	/**
	 * Everything the switches do, written to the debug output. Triggered by -help.
	 */
	void LogUsage();

	/**
	 * Whether -help, -h or -? was asked for
	 */
	_NODISCARD bool ShowHelpDocs();
}

#endif // DXI_COMMANDLINE_H_INCLUDED
