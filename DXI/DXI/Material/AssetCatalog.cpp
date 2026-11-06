#include "AssetCatalog.h"
#include "Paths.h"

#include <Windows.h>
#include <algorithm>

namespace
{
	// Every entry of a directory matching Pattern. Directories are returned when
	// bDirectories is set, files otherwise, and "." and ".." are never returned.
	std::vector<std::wstring> FindEntries(const std::wstring& Directory, const wchar_t* Pattern, const bool bDirectories)
	{
		std::vector<std::wstring> Found;

		WIN32_FIND_DATAW FindData	= {};
		const std::wstring Query	= Directory + L"\\" + Pattern;

		HANDLE Handle = FindFirstFileW(Query.c_str(), &FindData);
		if (Handle == INVALID_HANDLE_VALUE)
		{
			return Found;
		}

		do
		{
			const bool bIsDirectory = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			if (bIsDirectory != bDirectories)
			{
				continue;
			}

			const std::wstring Name = FindData.cFileName;
			if (Name == L"." || Name == L"..")
			{
				continue;
			}

			Found.push_back(Name);
		}
		while (FindNextFileW(Handle, &FindData) != 0);

		FindClose(Handle);

		std::sort(Found.begin(), Found.end());
		return Found;
	}

	bool HasAnyFile(const std::wstring& Directory, const wchar_t* Pattern)
	{
		return !FindEntries(Directory, Pattern, false).empty();
	}
}

void AssetCatalog::Scan()
{
	Models.clear();
	Materials.clear();

	// Material folders sit directly under the content root. A folder counts as one when
	// it holds a diffuse map named the Poly Haven way, which is what LoadMaterial expects.
	{
		const auto ContentRoot = DXIPATHS::GetTextureDir().Str();

		for (const auto& FolderName : FindEntries(ContentRoot, L"*", true))
		{
			const std::wstring FolderPath = ContentRoot + L"\\" + FolderName;

			if (HasAnyFile(FolderPath, (FolderName + L"_diff_1k.png").c_str()))
			{
				MaterialAsset Asset;
				Asset.Name = FolderName;
				Materials.push_back(std::move(Asset));
			}
		}
	}

	// Models live one folder deep under models/, with their maps beside them
	{
		const auto ModelRoot = DXIPATHS::GetModelDir().Str();

		for (const auto& FolderName : FindEntries(ModelRoot, L"*", true))
		{
			const std::wstring FolderPath = ModelRoot + L"\\" + FolderName;

			const auto MeshFiles = FindEntries(FolderPath, L"*.obj", false);
			if (MeshFiles.empty())
			{
				continue;
			}

			ModelAsset Model;
			Model.Name		= FolderName;
			Model.MeshPath	= FolderPath + L"\\" + MeshFiles.front();
			Models.push_back(std::move(Model));

			// A texture sitting beside the mesh becomes a material in its own right, so
			// the model's own colour map shows up in the picker like any other choice
			const auto TextureFiles = FindEntries(FolderPath, L"*.png", false);
			if (!TextureFiles.empty())
			{
				MaterialAsset Asset;
				Asset.Name				= FolderName;
				Asset.DiffuseOnlyPath	= FolderPath + L"\\" + TextureFiles.front();
				Materials.push_back(std::move(Asset));
			}
		}
	}
}
