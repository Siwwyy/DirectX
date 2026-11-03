#include "ObjLoader.h"

#include <fstream>
#include <sstream>

using namespace DirectX;

namespace
{
	// One corner of a face, as the file spells it: indices into the three source arrays.
	// Zero means the file left that channel out, since OBJ numbers from one.
	struct FaceCorner
	{
		int Position	= 0;
		int TexCoord	= 0;
		int Normal		= 0;
	};

	// OBJ indices count from one, and a negative index counts back from the end of what
	// has been read so far. Returns a zero based index, or -1 when the channel is absent.
	int ResolveIndex(const int Raw, const size_t Count)
	{
		if (Raw > 0)
		{
			return Raw - 1;
		}

		if (Raw < 0)
		{
			return static_cast<int>(Count) + Raw;
		}

		return -1;
	}

	// Parses "12", "12/34", "12//56" or "12/34/56"
	FaceCorner ParseFaceCorner(const std::string& Token)
	{
		FaceCorner Corner;

		// Reading into a stringstream with '/' swapped for spaces would lose the
		// difference between "12//56" and "12/56", so the slashes are walked by hand
		size_t Cursor	= 0;
		int Channel		= 0;

		while (Cursor <= Token.size() && Channel < 3)
		{
			const size_t Slash	= Token.find('/', Cursor);
			const size_t End	= Slash == std::string::npos ? Token.size() : Slash;

			if (End > Cursor)
			{
				const int Value = std::atoi(Token.substr(Cursor, End - Cursor).c_str());

				switch (Channel)
				{
				case 0:		Corner.Position = Value;	break;
				case 1:		Corner.TexCoord = Value;	break;
				default:	Corner.Normal	= Value;	break;
				}
			}

			if (Slash == std::string::npos)
			{
				break;
			}

			Cursor = Slash + 1;
			++Channel;
		}

		return Corner;
	}
}

LoadedMesh LoadObjFromFile(const std::wstring& Path)
{
	std::ifstream File(Path);
	if (!File.is_open())
	{
		throw std::runtime_error("Could not open OBJ file, check the model folder sits next to the executable");
	}

	std::vector<XMFLOAT3> Positions;
	std::vector<XMFLOAT2> TexCoords;

	// Every triangle keeps its own three corners, see the note in the header
	std::vector<FaceCorner> Corners;

	std::string Line;
	while (std::getline(File, Line))
	{
		// Strip a trailing carriage return, the archives are often zipped on another OS
		if (!Line.empty() && Line.back() == '\r')
		{
			Line.pop_back();
		}

		std::istringstream Stream(Line);
		std::string Keyword;
		if (!(Stream >> Keyword))
		{
			continue;
		}

		if (Keyword == "v")
		{
			XMFLOAT3 Position(0.f, 0.f, 0.f);
			Stream >> Position.x >> Position.y >> Position.z;
			Positions.push_back(Position);
		}
		else if (Keyword == "vt")
		{
			XMFLOAT2 TexCoord(0.f, 0.f);
			Stream >> TexCoord.x >> TexCoord.y;

			// OBJ measures v from the bottom, D3D from the top
			TexCoord.y = 1.f - TexCoord.y;
			TexCoords.push_back(TexCoord);
		}
		else if (Keyword == "f")
		{
			std::vector<FaceCorner> Polygon;

			std::string Token;
			while (Stream >> Token)
			{
				Polygon.push_back(ParseFaceCorner(Token));
			}

			// Fan a polygon of any size into triangles around its first corner
			for (size_t CornerIdx = 2; CornerIdx < Polygon.size(); ++CornerIdx)
			{
				Corners.push_back(Polygon[0]);
				Corners.push_back(Polygon[CornerIdx - 1]);
				Corners.push_back(Polygon[CornerIdx]);
			}
		}
		// v n, materials, groups and smoothing are all skipped on purpose
	}

	DXASSERT(Corners.size() % 3 == 0, "Triangles come in threes, the fan above should guarantee it");

	if (Corners.empty())
	{
		throw std::runtime_error("OBJ file holds no triangles");
	}

	LoadedMesh Mesh;
	Mesh.Vertices.resize(Corners.size());
	Mesh.Indices.resize(Corners.size());

	for (size_t TriangleIdx = 0; TriangleIdx < Corners.size() / 3; ++TriangleIdx)
	{
		const size_t Base = TriangleIdx * 3;

		XMFLOAT3 TrianglePositions[3]	= {};
		XMFLOAT2 TriangleTexCoords[3]	= {};

		for (size_t Corner = 0; Corner < 3; ++Corner)
		{
			const auto& Source = Corners[Base + Corner];

			const int PositionIdx = ResolveIndex(Source.Position, Positions.size());
			DXASSERT(PositionIdx >= 0 && PositionIdx < static_cast<int>(Positions.size()), "Face refers to a vertex the file never declared");
			TrianglePositions[Corner] = Positions[static_cast<size_t>(PositionIdx)];

			// A mesh with no uvs still has to come out drawable, it just lands on one texel
			const int TexCoordIdx = ResolveIndex(Source.TexCoord, TexCoords.size());
			if (TexCoordIdx >= 0 && TexCoordIdx < static_cast<int>(TexCoords.size()))
			{
				TriangleTexCoords[Corner] = TexCoords[static_cast<size_t>(TexCoordIdx)];
			}
		}

		// Same solver the built in primitives use, so a loaded mesh shades identically
		const auto Frame = ComputeTriangleFrame(TrianglePositions[0], TrianglePositions[1], TrianglePositions[2],
												TriangleTexCoords[0], TriangleTexCoords[1], TriangleTexCoords[2]);

		for (size_t Corner = 0; Corner < 3; ++Corner)
		{
			auto& Vertex = Mesh.Vertices[Base + Corner];

			Vertex.Position		= XMFLOAT4(TrianglePositions[Corner].x, TrianglePositions[Corner].y, TrianglePositions[Corner].z, 1.f);
			Vertex.TexCoord		= TriangleTexCoords[Corner];
			Vertex.Normal		= Frame.Normal;
			Vertex.Tangent		= Frame.Tangent;
			Vertex.Bitangent	= Frame.Bitangent;

			Mesh.Indices[Base + Corner] = static_cast<TypeOfIndice>(Base + Corner);
		}
	}

	return Mesh;
}
