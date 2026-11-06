#include "CommandLine.h"

#include <Windows.h>
#include <shellapi.h>
#include <algorithm>
#include <vector>

#pragma comment(lib, "shell32.lib")

namespace
{
	struct Switch
	{
		std::wstring Name;		// lower case, leading dashes stripped
		std::wstring Value;		// empty when the switch is a bare flag
	};

	std::vector<Switch>	Switches;
	bool				bParsed = false;

	std::wstring ToLower(std::wstring Text)
	{
		std::transform(Text.begin(), Text.end(), Text.begin(),
						[](const wchar_t Character) { return static_cast<wchar_t>(towlower(Character)); });
		return Text;
	}

	// Accepts -name, --name and /name so nobody has to remember which this engine wanted
	bool IsSwitchToken(const std::wstring& Token)
	{
		return !Token.empty() && (Token[0] == L'-' || Token[0] == L'/');
	}

	std::wstring StripPrefix(const std::wstring& Token)
	{
		size_t Start = 0;
		while (Start < Token.size() && (Token[Start] == L'-' || Token[Start] == L'/'))
		{
			++Start;
		}
		return Token.substr(Start);
	}

	const Switch* Find(const wchar_t* Name)
	{
		if (!bParsed)
		{
			// Reading a switch before Parse() would silently answer "absent" for everything
			DXASSERT(bParsed, "DXICMD::Parse must be called before any switch is read");
			return nullptr;
		}

		const std::wstring Wanted = ToLower(Name ? Name : L"");

		for (const auto& Entry : Switches)
		{
			if (Entry.Name == Wanted)
			{
				return &Entry;
			}
		}

		return nullptr;
	}
}

void DXICMD::Parse()
{
	if (bParsed)
	{
		return;
	}

	bParsed = true;

	int ArgCount		= 0;
	LPWSTR* Arguments	= CommandLineToArgvW(GetCommandLineW(), &ArgCount);
	if (!Arguments)
	{
		return;
	}

	// Index 0 is the executable path
	for (int ArgIdx = 1; ArgIdx < ArgCount; ++ArgIdx)
	{
		std::wstring Token = Arguments[ArgIdx];
		if (!IsSwitchToken(Token))
		{
			continue;
		}

		Switch Entry;
		Token = StripPrefix(Token);

		// -name=value
		const size_t Equals = Token.find(L'=');
		if (Equals != std::wstring::npos)
		{
			Entry.Name	= ToLower(Token.substr(0, Equals));
			Entry.Value	= Token.substr(Equals + 1);
		}
		else
		{
			Entry.Name = ToLower(Token);

			// -name value, but only when what follows is not itself a switch, so a bare
			// flag sitting in front of another flag is not mistaken for taking a value
			if (ArgIdx + 1 < ArgCount && !IsSwitchToken(Arguments[ArgIdx + 1]))
			{
				Entry.Value = Arguments[ArgIdx + 1];
				++ArgIdx;
			}
		}

		Switches.push_back(std::move(Entry));
	}

	LocalFree(Arguments);
}

bool DXICMD::HasFlag(const wchar_t* Name)
{
	return Find(Name) != nullptr;
}

int DXICMD::GetInt(const wchar_t* Name, const int Fallback)
{
	const Switch* const Entry = Find(Name);
	if (!Entry || Entry->Value.empty())
	{
		return Fallback;
	}

	try
	{
		return std::stoi(Entry->Value);
	}
	catch (...)
	{
		// A switch given a word where a number belongs is worth saying out loud rather
		// than quietly running at the default
		DXLOG("Command line: value of the switch is not a number, using the default\n")
		return Fallback;
	}
}

bool DXICMD::WantsUsage()
{
	return HasFlag(L"help") || HasFlag(L"h") || HasFlag(L"?");
}

void DXICMD::LogUsage()
{
	DXLOG("\nDXIApp switches | -name, --name or /name, case insensitive\n")
	DXLOG("\n  Debug layer\n")
	DXLOG("    -gpuvalidation     GPU based validation. Instruments every shader, commonly 10x slower.\n")
	DXLOG("    -nodebuglayer      Skip the debug layer entirely. Needed when Graphics Tools is not installed.\n")
	DXLOG("    -nobreak           Do not break into the debugger on debug layer messages.\n")
	DXLOG("\n  Adapter\n")
	DXLOG("    -warp              Render on the WARP software adapter, which tells a driver bug from an engine bug.\n")
	DXLOG("\n  Presentation\n")
	DXLOG("    -vsync             Present with a sync interval of 1. Off by default.\n")
	DXLOG("    -width  N          Window width in pixels.\n")
	DXLOG("    -height N          Window height in pixels.\n")
	DXLOG("\n  -help, -?          This text\n\n")
}
