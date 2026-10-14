//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef DXIPATHS_H_INCLUDED
#define DXIPATHS_H_INCLUDED

#include <string>

// Absolute asset paths, independent of the process working directory.
//
//     DXIPATHS::GetShaderDir / L"vertex_shader.hlsl"
//     DXIPATHS::GetShaderDir / L"BasePass" / L"vertex_shader.hlsl"
//     DXIPATHS::GetTextureDir / L"bryanzar.png"
//
// The joined path is absolute. FilePath converts to const wchar_t* for the
// lifetime of the object. Call Str() when the API wants a std::wstring.

namespace DXIPATHS
{
	class FilePath
	{
	public:

		FilePath() = default;
		explicit FilePath(std::wstring InPath);

		FilePath operator/(const wchar_t* Relative) const;
		FilePath operator/(const char* Relative) const;
		FilePath operator/(const std::wstring& Relative) const;

		const wchar_t* c_str() const noexcept { return Path.c_str(); }
		const std::wstring& Str() const noexcept { return Path; }

		// Valid while this FilePath is alive. Prefer Str() when a std::wstring is required.
		operator const wchar_t*() const noexcept { return Path.c_str(); }

	private:

		std::wstring Path;
	};


	class Directory
	{
	public:

		enum class Kind
		{
			Executable,
			Content,
			Shader,
			Texture
		};

		constexpr explicit Directory(Kind InKind) noexcept
			: DirectoryKind(InKind)
		{
		}

		// Directory path itself: DXIPATHS::GetShaderDir()
		FilePath operator()() const;

		FilePath operator/(const wchar_t* Relative) const;
		FilePath operator/(const char* Relative) const;
		FilePath operator/(const std::wstring& Relative) const;

	private:

		FilePath Resolve() const;

		Kind DirectoryKind;
	};


	// Folder of the running executable.
	extern const Directory GetExecutableDir;

	// DXI content root: the folder that contains shaders and loose textures.
	extern const Directory GetContentDir;

	// <content>/shaders
	extern const Directory GetShaderDir;

	// Folder that holds texture files. Today that is the content root.
	extern const Directory GetTextureDir;
}

inline DXIPATHS::FilePath DXIPATHS::FilePath::operator/(const std::wstring& Relative) const
{
	return *this / Relative.c_str();
}

inline DXIPATHS::FilePath DXIPATHS::Directory::operator/(const std::wstring& Relative) const
{
	return *this / Relative.c_str();
}

#endif // DXIPATHS_H_INCLUDED
