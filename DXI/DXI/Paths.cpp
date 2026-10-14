//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "Paths.h"

#include <Windows.h>
#include <cassert>
#include <cstring>

namespace
{
	bool IsDirectory(const std::wstring& Path)
	{
		const DWORD Attributes = GetFileAttributesW(Path.c_str());
		return Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	std::wstring ParentOf(std::wstring Path)
	{
		while (!Path.empty() && (Path.back() == L'\\' || Path.back() == L'/'))
		{
			Path.pop_back();
		}

		const std::wstring::size_type Slash = Path.find_last_of(L"\\/");
		if (Slash == std::wstring::npos)
		{
			return {};
		}

		// Keep the drive root ("D:\") so the walk can stop cleanly.
		if (Slash == 2 && Path[1] == L':')
		{
			return Path.substr(0, Slash + 1);
		}

		if (Slash == 0)
		{
			return Path.substr(0, 1);
		}

		return Path.substr(0, Slash);
	}

	std::wstring Canonical(const std::wstring& Path)
	{
		if (Path.empty())
		{
			return Path;
		}

		const DWORD Required = GetFullPathNameW(Path.c_str(), 0, nullptr, nullptr);
		if (Required == 0)
		{
			return Path;
		}

		std::wstring Full(Required, L'\0');
		const DWORD Written = GetFullPathNameW(Path.c_str(), Required, &Full[0], nullptr);
		if (Written == 0 || Written >= Required)
		{
			return Path;
		}

		Full.resize(Written);
		return Full;
	}

	std::wstring ExecutableDirectory()
	{
		std::wstring Buffer(MAX_PATH, L'\0');
		for (;;)
		{
			const DWORD Length = GetModuleFileNameW(nullptr, &Buffer[0], static_cast<DWORD>(Buffer.size()));
			if (Length == 0)
			{
				return {};
			}

			if (Length < Buffer.size())
			{
				Buffer.resize(Length);
				break;
			}

			Buffer.resize(Buffer.size() * 2);
		}

		return ParentOf(Buffer);
	}

	std::wstring CurrentDirectory()
	{
		const DWORD Required = GetCurrentDirectoryW(0, nullptr);
		if (Required == 0)
		{
			return {};
		}

		std::wstring Buffer(Required, L'\0');
		const DWORD Written = GetCurrentDirectoryW(Required, &Buffer[0]);
		if (Written == 0 || Written >= Required)
		{
			return {};
		}

		Buffer.resize(Written);
		return Buffer;
	}

	// Content root is the directory that contains "shaders", or a parent that
	// contains "DXI/shaders" (repo root -> DXI project folder).
	std::wstring FindContentFrom(std::wstring Current)
	{
		for (int Step = 0; Step < 10 && !Current.empty(); ++Step)
		{
			if (IsDirectory(Current + L"\\shaders"))
			{
				return Current;
			}

			if (IsDirectory(Current + L"\\DXI\\shaders"))
			{
				return Current + L"\\DXI";
			}

			const std::wstring Parent = ParentOf(Current);
			if (Parent.empty() || Parent == Current)
			{
				break;
			}

			Current = Parent;
		}

		return {};
	}

	const std::wstring& ContentDirectory()
	{
		static const std::wstring Cached = []() -> std::wstring
		{
			std::wstring Found = FindContentFrom(ExecutableDirectory());
			if (Found.empty())
			{
				Found = FindContentFrom(CurrentDirectory());
			}

			assert(!Found.empty() && "DXIPATHS could not locate the DXI content directory that contains shaders");
			if (Found.empty())
			{
				return ExecutableDirectory();
			}

			return Canonical(Found);
		}();

		return Cached;
	}
}

DXIPATHS::FilePath::FilePath(std::wstring InPath)
	: Path(Canonical(std::move(InPath)))
{
}

DXIPATHS::FilePath DXIPATHS::FilePath::operator/(const wchar_t* Relative) const
{
	if (Relative == nullptr || Relative[0] == L'\0')
	{
		return *this;
	}

	while (*Relative == L'\\' || *Relative == L'/')
	{
		++Relative;
	}

	std::wstring Combined = Path;
	if (!Combined.empty() && Combined.back() != L'\\' && Combined.back() != L'/')
	{
		Combined.push_back(L'\\');
	}

	for (const wchar_t* Cursor = Relative; *Cursor != L'\0'; ++Cursor)
	{
		Combined.push_back(*Cursor == L'/' ? L'\\' : *Cursor);
	}

	return FilePath(std::move(Combined));
}

DXIPATHS::FilePath DXIPATHS::FilePath::operator/(const char* Relative) const
{
	if (Relative == nullptr || Relative[0] == '\0')
	{
		return *this;
	}

	std::wstring Wide;
	Wide.reserve(std::strlen(Relative));
	for (const char* Cursor = Relative; *Cursor != '\0'; ++Cursor)
	{
		Wide.push_back(static_cast<unsigned char>(*Cursor));
	}

	return *this / Wide.c_str();
}

DXIPATHS::FilePath DXIPATHS::Directory::operator()() const
{
	return Resolve();
}

DXIPATHS::FilePath DXIPATHS::Directory::operator/(const wchar_t* Relative) const
{
	return Resolve() / Relative;
}

DXIPATHS::FilePath DXIPATHS::Directory::operator/(const char* Relative) const
{
	return Resolve() / Relative;
}

DXIPATHS::FilePath DXIPATHS::Directory::Resolve() const
{
	switch (DirectoryKind)
	{
	case Kind::Executable:
		return FilePath(ExecutableDirectory());
	case Kind::Content:
		return FilePath(ContentDirectory());
	case Kind::Shader:
		return FilePath(ContentDirectory() + L"\\shaders");
	case Kind::Texture:
		return FilePath(ContentDirectory());
	default:
		return {};
	}
}

const DXIPATHS::Directory DXIPATHS::GetExecutableDir{ DXIPATHS::Directory::Kind::Executable };
const DXIPATHS::Directory DXIPATHS::GetContentDir{ DXIPATHS::Directory::Kind::Content };
const DXIPATHS::Directory DXIPATHS::GetShaderDir{ DXIPATHS::Directory::Kind::Shader };
const DXIPATHS::Directory DXIPATHS::GetTextureDir{ DXIPATHS::Directory::Kind::Texture };
