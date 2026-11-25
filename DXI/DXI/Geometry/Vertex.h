//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_VERTEX_H_INCLUDED
#define D3D12_VERTEX_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"

#include <algorithm>
#include <cfloat>

// usings
using TypeOfIndice = DWORD;

/** Supported vertex layouts, one for each of the Vertex structs below. */
enum class VertexType : UINT
{
	Position,							// XMFLOAT4
	PositionNormal,						// XMFLOAT4 + XMFLOAT4
	PositionColor,						// XMFLOAT4 + XMFLOAT4
	PositionColorNormal,				// XMFLOAT4 + XMFLOAT4 + XMFLOAT4
	PositionUV,							// XMFLOAT4 + XMFLOAT2
	PositionUVNormal,					// XMFLOAT4 + XMFLOAT2 + XMFLOAT4
	PositionUVNormalTangent,			// XMFLOAT4 + XMFLOAT2 + XMFLOAT4 + XMFLOAT4
	PositionUVNormalTangentBitangent,	// XMFLOAT4 + XMFLOAT2 + XMFLOAT4 + XMFLOAT4 + XMFLOAT4

	VertexTypeCount						// Number of layouts, the arrays below are indexed by this
};

/** Position only, 16 bytes. Matches VertexType::Position. */
struct VertexPosition
{
	constexpr static VertexType VertexTypeValue = VertexType::Position;
	constexpr static uint32_t InputElementCount = 1;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{ 
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } 
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1

	/** Position (x,y,z[,pw]). */
	VertexPosition(float x = 0.0f, float y = 0.0f, float z = 0.0f, float pw = 1.0f) noexcept
		: Position(x, y, z, pw)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPosition) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(DirectX::XMFLOAT4) <= Data.size(), L"Insufficient buffer size for VertexPosition deserialization");
		std::memcpy(&Position,	&Data[0],	sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPosition));
		std::memcpy(&Data[0],	&Position,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/** Position and normal, 32 bytes. Matches VertexType::PositionNormal. */
struct VertexPositionNormal
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionNormal;
	constexpr static uint32_t InputElementCount = 2;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1
	DirectX::XMFLOAT4 Normal;	// Surface normal, W unused

	/** Position (x,y,z[,pw]) and normal (nx,ny,nz[,nw]). */
	VertexPositionNormal(float x = 0.0f, float y = 0.0f, float z = 0.0f,
						float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
						float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, Normal(nx, ny, nz, nw)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPositionNormal) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionNormal deserialization");
		std::memcpy(&Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionNormal));
		std::memcpy(&Data[0],							&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],	&Normal,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/** Position and colour, 32 bytes. Matches VertexType::PositionColor. */
struct VertexPositionColor
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionColor;
	constexpr static uint32_t InputElementCount = 2;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1
	DirectX::XMFLOAT4 Color;	// RGBA colour

	/** Position (x,y,z[,pw]) and colour (r,g,b,a). */
	VertexPositionColor(float x = 0.0f, float y = 0.0f, float z = 0.0f,
						float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
						float pw = 1.0f) noexcept
		: Position(x, y, z, pw)
		, Color(r, g, b, a)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPositionColor) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionColor) <= Data.size(), L"Insufficient buffer size for VertexPositionColor deserialization");
		std::memcpy(&Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Color,		&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionColor));
		std::memcpy(&Data[0],							&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],	&Color,		sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/** Position, colour and normal, 48 bytes. Matches VertexType::PositionColorNormal. */
struct VertexPositionColorNormal
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionColorNormal;
	constexpr static uint32_t InputElementCount = 3;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1
	DirectX::XMFLOAT4 Color;	// RGBA colour
	DirectX::XMFLOAT4 Normal;	// Surface normal, W unused

	/** Position (x,y,z[,pw]), colour (r,g,b,a) and normal (nx,ny,nz[,nw]). */
	VertexPositionColorNormal(float x = 0.0f, float y = 0.0f, float z = 0.0f,
								float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
								float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
								float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, Color(r, g, b, a)
		, Normal(nx, ny, nz, nw)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPositionColorNormal) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionColorNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionColorNormal deserialization");
		std::memcpy(&Position,	&Data[0],														sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Color,		&Data[sizeof(DirectX::XMFLOAT4)],								sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionColorNormal));
		std::memcpy(&Data[0],														&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],								&Color,		sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT4)],	&Normal,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/** Position and UV, 24 bytes. Matches VertexType::PositionUV. */
struct VertexPositionUV
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionUV;
	constexpr static uint32_t InputElementCount = 2;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1
	DirectX::XMFLOAT2 UV;		// Texture coordinates

	/** Position (x,y,z[,pw]) and UV (u,v). */
	VertexPositionUV(float x = 0.0f, float y = 0.0f, float z = 0.0f,
							float u = 0.0f, float v = 0.0f,
							float pw = 1.0f) noexcept
		: Position(x, y, z, pw)
		, UV(u, v)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPositionUV) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionUV) <= Data.size(), L"Insufficient buffer size for VertexPositionUV deserialization");
		std::memcpy(&Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&UV,	&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT2));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionUV));
		std::memcpy(&Data[0],							&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],	&UV,	sizeof(DirectX::XMFLOAT2));
		return Data;
	}
};

/** Position, UV and normal, 40 bytes. Matches VertexType::PositionUVNormal. */
struct VertexPositionUVNormal
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionUVNormal;
	constexpr static uint32_t InputElementCount = 3;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION",	0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,		DXGI_FORMAT_R32G32_FLOAT,		0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1
	DirectX::XMFLOAT2 UV;		// Texture coordinates
	DirectX::XMFLOAT4 Normal;	// Surface normal, W unused

	/** Position (x,y,z[,pw]), UV (u,v) and normal (nx,ny,nz[,nw]). */
	VertexPositionUVNormal(float x = 0.0f, float y = 0.0f, float z = 0.0f,
								float u = 0.0f, float v = 0.0f,
								float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
								float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, UV(u, v)
		, Normal(nx, ny, nz, nw)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPositionUVNormal) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionUVNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionUVNormal deserialization");
		std::memcpy(&Position,	&Data[0],														sizeof(DirectX::XMFLOAT4));
		std::memcpy(&UV,		&Data[sizeof(DirectX::XMFLOAT4)],								sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],	sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionUVNormal));
		std::memcpy(&Data[0],														&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],								&UV,		sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],	&Normal,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/**
 * Position, UV, normal and tangent, 56 bytes. Matches VertexType::PositionUVNormalTangent.
 * The tangent is what a tangent space normal map needs, since the map stores directions
 * relative to the surface. Meshes that are never normal mapped want VertexPositionUVNormal
 * instead, which is 16 bytes a vertex lighter.
 */
struct VertexPositionUVNormalTangent
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionUVNormalTangent;
	constexpr static uint32_t InputElementCount = 4;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION",	0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,		DXGI_FORMAT_R32G32_FLOAT,		0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;	// 3D position, W is typically 1
	DirectX::XMFLOAT2 UV;		// Texture coordinates
	DirectX::XMFLOAT4 Normal;	// Surface normal, W unused
	DirectX::XMFLOAT4 Tangent;	// Along growing U, W is the sign the bitangent is flipped by

	/**
	 * Position (x,y,z[,pw]), UV (u,v) and normal (nx,ny,nz[,nw]). The tangent defaults to
	 * the x axis, ComputeFaceNormalTangent fills in the real one.
	 */
	VertexPositionUVNormalTangent(float x = 0.0f, float y = 0.0f, float z = 0.0f,
										float u = 0.0f, float v = 0.0f,
										float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
										float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, UV(u, v)
		, Normal(nx, ny, nz, nw)
		, Tangent(1.0f, 0.0f, 0.0f, 1.0f)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized vertex data, at least sizeof(VertexPositionUVNormalTangent) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionUVNormalTangent) <= Data.size(), L"Insufficient buffer size for VertexPositionUVNormalTangent deserialization");
		std::memcpy(&Position,	&Data[0],																			sizeof(DirectX::XMFLOAT4));
		std::memcpy(&UV,		&Data[sizeof(DirectX::XMFLOAT4)],													sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],						sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Tangent,	&Data[(sizeof(DirectX::XMFLOAT4) * 2) + sizeof(DirectX::XMFLOAT2)],					sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionUVNormalTangent));
		std::memcpy(&Data[0],																			&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],													&UV,		sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],						&Normal,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[(sizeof(DirectX::XMFLOAT4) * 2) + sizeof(DirectX::XMFLOAT2)],					&Tangent,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/**
 * Position, UV, normal, tangent and bitangent, 72 bytes. Matches
 * VertexType::PositionUVNormalTangentBitangent. Carries the bitangent rather than deriving
 * it, which costs 16 bytes a vertex over VertexPositionUVNormalTangent and is only worth it
 * when the bitangent cannot be reconstructed, such as a sheared rather than mirrored UV shell.
 */
struct VertexPositionUVNormalTangentBitangent
{
	constexpr static VertexType VertexTypeValue = VertexType::PositionUVNormalTangentBitangent;
	constexpr static uint32_t InputElementCount = 5;
	constexpr static D3D12_INPUT_ELEMENT_DESC InputElementDesc[InputElementCount] =
	{
		{ "POSITION",	0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,		DXGI_FORMAT_R32G32_FLOAT,		0, 16,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL",	0,		DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DirectX::XMFLOAT4 Position;		// 3D position, W is typically 1
	DirectX::XMFLOAT2 UV;			// Texture coordinates
	DirectX::XMFLOAT4 Normal;		// Surface normal, W unused
	DirectX::XMFLOAT4 Tangent;		// Along growing U, W is the handedness as in the layout above
	DirectX::XMFLOAT4 Bitangent;	// Along growing V, W unused

	/**
	 * Position (x,y,z[,pw]), UV (u,v) and normal (nx,ny,nz[,nw]). Tangent and bitangent
	 * default to the x and y axes, ComputeFaceNormalTangent fills in the real ones.
	 */
	VertexPositionUVNormalTangentBitangent(float x = 0.0f, float y = 0.0f, float z = 0.0f,
												float u = 0.0f, float v = 0.0f,
												float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
												float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, UV(u, v)
		, Normal(nx, ny, nz, nw)
		, Tangent(1.0f, 0.0f, 0.0f, 1.0f)
		, Bitangent(0.0f, 1.0f, 0.0f, 0.0f)
	{
	}

	/**
	 * Populates this structure from a byte buffer.
	 *
	 * @param Data Serialized data, at least sizeof(VertexPositionUVNormalTangentBitangent) bytes
	 */
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionUVNormalTangentBitangent) <= Data.size(), L"Insufficient buffer size for VertexPositionUVNormalTangentBitangent deserialization");
		std::memcpy(&Position,	&Data[0],																			sizeof(DirectX::XMFLOAT4));
		std::memcpy(&UV,	&Data[sizeof(DirectX::XMFLOAT4)],														sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],						sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Tangent,	&Data[(sizeof(DirectX::XMFLOAT4) * 2) + sizeof(DirectX::XMFLOAT2)],					sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Bitangent,	&Data[(sizeof(DirectX::XMFLOAT4) * 3) + sizeof(DirectX::XMFLOAT2)],					sizeof(DirectX::XMFLOAT4));
	}

	/**
	 * Serializes this vertex into a byte buffer.
	 *
	 * @return Vector holding the serialized vertex data
	 */
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionUVNormalTangentBitangent));
		std::memcpy(&Data[0],																			&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],													&UV,		sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],						&Normal,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[(sizeof(DirectX::XMFLOAT4) * 2) + sizeof(DirectX::XMFLOAT2)],					&Tangent,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[(sizeof(DirectX::XMFLOAT4) * 3) + sizeof(DirectX::XMFLOAT2)],					&Bitangent,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

// The byte offsets in InputElementDesc above are written out by hand, so they have to be
// held against the members they claim to describe
static_assert(sizeof(VertexPositionUVNormalTangentBitangent) == 72, "Layout has changed, the input element offsets need revisiting");
static_assert(offsetof(VertexPositionUVNormalTangentBitangent, Tangent) == 40, "TANGENT is declared at offset 40");
static_assert(offsetof(VertexPositionUVNormalTangentBitangent, Bitangent) == 56, "BINORMAL is declared at offset 56");

/** Size of each vertex layout, indexed by VertexType. Drives allocation and validation. */
const uint64_t VertexTypeSizes[] = {
	sizeof(VertexPosition),									// VertexType::Position
	sizeof(VertexPositionNormal),							// VertexType::PositionNormal
	sizeof(VertexPositionColor),							// VertexType::PositionColor
	sizeof(VertexPositionColorNormal),						// VertexType::PositionColorNormal
	sizeof(VertexPositionUV),								// VertexType::PositionUV
	sizeof(VertexPositionUVNormal),							// VertexType::PositionUVNormal
	sizeof(VertexPositionUVNormalTangent),					// VertexType::PositionUVNormalTangent
	sizeof(VertexPositionUVNormalTangentBitangent)			// VertexType::PositionUVNormalTangentBitangent
};
static_assert(_countof(VertexTypeSizes) == static_cast<size_t>(VertexType::VertexTypeCount),
				"Every VertexType needs its size listed here, the array is indexed by the enum");

/** Whether a layout stores its own bitangent, rather than leaving the shader to derive it. */
template<typename T, typename = void>
struct HasBitangent : std::false_type {};

template<typename T>
struct HasBitangent<T, std::void_t<decltype(T::Bitangent)>> : std::true_type {};

/** The basis one triangle gives its vertices. */
struct TriangleFrame
{
	DirectX::XMFLOAT4 Normal;		// Plane of the triangle, W unused
	DirectX::XMFLOAT4 Tangent;		// Along growing U, W is the handedness
	DirectX::XMFLOAT4 Bitangent;	// Along growing V, W unused
};

/**
 * Solves one triangle for its normal, tangent and bitangent. The tangent is the direction U
 * grows in, found by solving the two edges against their UV deltas, the bitangent the same
 * for V with its normal component removed so a sheared UV keeps the angle between the two.
 * Shared with the mesh loader, which has no compile time vertex count to template on.
 *
 * @return The basis, with the handedness in Tangent.w
 */
_NODISCARD TriangleFrame ComputeTriangleFrame(const DirectX::XMFLOAT3& Position0,
											const DirectX::XMFLOAT3& Position1,
											const DirectX::XMFLOAT3& Position2,
											const DirectX::XMFLOAT2& UV0,
											const DirectX::XMFLOAT2& UV1,
											const DirectX::XMFLOAT2& UV2);

/**
 * Per face normal, tangent and bitangent for every vertex of a mesh, the basis a tangent
 * space normal map is expressed in. The bitangent is only written for layouts with somewhere
 * to put it, see HasBitangent above. Flat faces on purpose: all three corners of a triangle
 * are given the same frame, so a mesh wanting smooth shading must supply its own normals.
 *
 * @param VertexArray Source vertices, read for position and UV
 * @param IndicesList Triangle list, three indices per face
 * @return The vertices with their frames filled in
 */
template<typename In, typename Out, size_t N, size_t M>
std::array<Out, N> ComputeFaceNormalTangent(const In (&VertexArray)[N], const TypeOfIndice (&IndicesList)[M])
{
	constexpr size_t NumVerticesPerTriangle	= 3;
	constexpr size_t NumTriangles			= M / NumVerticesPerTriangle;

	std::array<Out, N> out{};

	for (size_t TriangleIdx = 0; TriangleIdx < NumTriangles; ++TriangleIdx)
	{
		const TypeOfIndice Indices[3] =
		{
			IndicesList[TriangleIdx * NumVerticesPerTriangle + 0],
			IndicesList[TriangleIdx * NumVerticesPerTriangle + 1],
			IndicesList[TriangleIdx * NumVerticesPerTriangle + 2]
		};

		const In& Vertex0 = VertexArray[Indices[0]];
		const In& Vertex1 = VertexArray[Indices[1]];
		const In& Vertex2 = VertexArray[Indices[2]];

		const auto Frame = ComputeTriangleFrame(
			DirectX::XMFLOAT3(Vertex0.Position.x, Vertex0.Position.y, Vertex0.Position.z),
			DirectX::XMFLOAT3(Vertex1.Position.x, Vertex1.Position.y, Vertex1.Position.z),
			DirectX::XMFLOAT3(Vertex2.Position.x, Vertex2.Position.y, Vertex2.Position.z),
			Vertex0.UV, Vertex1.UV, Vertex2.UV);

		for (const TypeOfIndice Index : Indices)
		{
			out[Index].Position	= DirectX::XMFLOAT4(VertexArray[Index].Position.x, VertexArray[Index].Position.y, VertexArray[Index].Position.z, 1.f);
			out[Index].UV	= DirectX::XMFLOAT2(VertexArray[Index].UV.x, VertexArray[Index].UV.y);
			out[Index].Normal	= Frame.Normal;
			out[Index].Tangent	= Frame.Tangent;

			if constexpr (HasBitangent<Out>::value)
			{
				out[Index].Bitangent = Frame.Bitangent;
			}
		}
	}

	return out;
}

template<typename In, typename Out, size_t N, size_t M>
std::array<Out, N> ComputeFaceNormal(const In (&VertexArray)[N], const TypeOfIndice (&IndicesList)[M])
{
	constexpr size_t NumVertices = N;	// Number of vertices in the input array
	constexpr size_t NumVerticesPerTriangle = 3;	// Number of vertices in triangle (everytime its 3, but who knows?)
	constexpr size_t NumTriangles = M / NumVerticesPerTriangle; // Assuming each triangle is defined by 3 indices
	std::array<Out, N> out{};

	for (size_t i = 0; i < NumTriangles; ++i)
	{
		// Get values
		const TypeOfIndice IdxX0	= IndicesList[i * NumVerticesPerTriangle + 0];
		const TypeOfIndice IdxX1	= IndicesList[i * NumVerticesPerTriangle + 1];
		const TypeOfIndice IdxX2	= IndicesList[i * NumVerticesPerTriangle + 2];
		const In& Vertex0	= VertexArray[IdxX0];
		const In& Vertex1	= VertexArray[IdxX1];
		const In& Vertex2	= VertexArray[IdxX2];
		const DirectX::XMVECTOR VertexPos0 = DirectX::XMVectorSet(Vertex0.Position.x, Vertex0.Position.y, Vertex0.Position.z, 0.0f);
		const DirectX::XMVECTOR VertexPos1 = DirectX::XMVectorSet(Vertex1.Position.x, Vertex1.Position.y, Vertex1.Position.z, 0.0f);
		const DirectX::XMVECTOR VertexPos2 = DirectX::XMVectorSet(Vertex2.Position.x, Vertex2.Position.y, Vertex2.Position.z, 0.0f);

		// Calculate normal using cross product of two edges of the triangle (assuming vertices are defined in a consistent winding order) see per Primitive Vertex List and Indices List
		// see https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-normals.html
		const DirectX::XMVECTOR Edge1	= DirectX::XMVectorSubtract(VertexPos1, VertexPos0);
		const DirectX::XMVECTOR Edge2	= DirectX::XMVectorSubtract(VertexPos2, VertexPos0);
		const DirectX::XMVECTOR Normal	= DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Edge1, Edge2));

#if 1
		//DXLOG("Triangle %zu: Normal = (%f, %f, %f)", i, XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal))
#endif 

		// SANITY CHECKS
		// Check if normal is not a zero vector
		// Create a zero vector to compare against
		XMVECTOR Epsilon	= XMVectorSplatEpsilon();
		XMVECTOR ZeroVector = XMVectorZero();

		// Perform the near equality check
		// The result is a control vector that can be tested
		bool IsNormalNearZero = XMVector3NearEqual(Normal, ZeroVector, Epsilon);
		DXASSERT(!IsNormalNearZero, "Normal vector can not be zero vector");

		// Check if normal length is equal to 1.0 (Unit Length Vector
		XMVECTOR UnitVector		= XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		bool IsNormalUnitLength = XMVector3NearEqual(XMVector3Length(Normal), XMVector3Length(UnitVector), Epsilon);
		DXASSERT(IsNormalUnitLength, "Normal vector should be unit length vector");

		// Serialize to bytes
		std::vector<BYTE> DataVertex0 = Vertex0.SerializeToBytes();
		std::vector<BYTE> DataVertex1 = Vertex1.SerializeToBytes();
		std::vector<BYTE> DataVertex2 = Vertex2.SerializeToBytes();

		// Create NormalXYZW for memcpy
		const float NormalXYZW[]	= { XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal), 0.0f };
		const size_t NormalSize		= sizeof(NormalXYZW);

		// Resize to fill Normals
		DataVertex0.resize(DataVertex0.size() + NormalSize);
		DataVertex1.resize(DataVertex1.size() + NormalSize);
		DataVertex2.resize(DataVertex2.size() + NormalSize);

		// Append NormalXYZW to the end of the vertex data
		std::memcpy(&DataVertex0[DataVertex0.size() - NormalSize], NormalXYZW, NormalSize);
		std::memcpy(&DataVertex1[DataVertex1.size() - NormalSize], NormalXYZW, NormalSize);
		std::memcpy(&DataVertex2[DataVertex2.size() - NormalSize], NormalXYZW, NormalSize);

		// Create Vertices with Normals
		Out Vertex0WithNormal;
		Out Vertex1WithNormal;
		Out Vertex2WithNormal;

		// Deserialize and initialize with new data | TODO Maybe rethink that part...
		Vertex0WithNormal.DeserializeFromBytes(DataVertex0);
		Vertex1WithNormal.DeserializeFromBytes(DataVertex1);
		Vertex2WithNormal.DeserializeFromBytes(DataVertex2);

		// Assign values
		out[IdxX0] = Vertex0WithNormal;
		out[IdxX1] = Vertex1WithNormal;
		out[IdxX2] = Vertex2WithNormal;
	}
	// Return
	return out;
}

/**
 * A mesh read off disk, in the layout the base pass expects. Shared by every loader, so the
 * format a mesh came out of stops mattering past the point it was parsed.
 */
struct LoadedMesh
{
	std::vector<VertexPositionUVNormalTangentBitangent>	Vertices;
	std::vector<TypeOfIndice>									Indices;
};

/**
 * Everything VertexFactory::Init needs: the layout, its input elements, and the vertex and
 * index buffers. Move only, so a large mesh is handed over rather than copied.
 */
template<typename TypeOfVertex = VertexPosition>
struct VertexInitData
{
	DISABLE_COPY(VertexInitData)
	ENABLE_MOVE(VertexInitData)

	// Constants
	constexpr static size_t MaxVertexDataSize	= 2 << 10;		// Maximum size of vertex data in bytes (adjust as needed)
	constexpr static size_t MaxIndexDataSize	= 2 << 10;		// Maximum size of index data in bytes (adjust as needed)
	constexpr static size_t MaxElementDescSize	= 3;			// Maximum number of input element descriptions (adjust as needed)

	// Make fields movable by removing const qualifiers.
	VertexType									Type{};
	std::vector<D3D12_INPUT_ELEMENT_DESC>		InputElementDesc{};
	std::vector<TypeOfVertex>					VertexData{};
	std::vector<TypeOfIndice>					IndexData{};

	// Make function to initialize init data for vertex.
	template<std::size_t NumVertices>
	static VertexInitData<TypeOfVertex> CreateVertexInitData(
		const std::array<TypeOfVertex, NumVertices>& Vertices, 
		TypeOfIndice const * const IndicesArray,
		const std::size_t NumIndices)
	{
		VertexInitData<TypeOfVertex> InitData;
		InitData.Type = TypeOfVertex::VertexTypeValue;

		// Input Element Desc
		InitData.InputElementDesc.insert(
			InitData.InputElementDesc.end(),
			TypeOfVertex::InputElementDesc,
			TypeOfVertex::InputElementDesc + TypeOfVertex::InputElementCount
		);

		// Vertex Data
		InitData.VertexData.insert(
			InitData.VertexData.end(),
			Vertices.begin(),
			Vertices.end()
		);

		// Index Data
		InitData.IndexData.insert(
			InitData.IndexData.end(),
			IndicesArray,
			IndicesArray + NumIndices
		);

		// Return
		return InitData; //RVO / NRVO
	}

	// Runtime sized sibling of the above, for a mesh read off disk where the counts are
	// not known until the file has been parsed. Takes the buffers by value and moves them
	// straight in, so a large mesh is never copied.
	static VertexInitData<TypeOfVertex> CreateVertexInitData(
		std::vector<TypeOfVertex> Vertices,
		std::vector<TypeOfIndice> Indices)
	{
		DXASSERT(!Vertices.empty(), "A mesh with no vertices can not be uploaded");
		DXASSERT(!Indices.empty(), "A mesh with no indices can not be drawn");

		VertexInitData<TypeOfVertex> InitData;
		InitData.Type = TypeOfVertex::VertexTypeValue;

		InitData.InputElementDesc.insert(
			InitData.InputElementDesc.end(),
			TypeOfVertex::InputElementDesc,
			TypeOfVertex::InputElementDesc + TypeOfVertex::InputElementCount
		);

		InitData.VertexData	= std::move(Vertices);
		InitData.IndexData	= std::move(Indices);

		return InitData;
	}

	// Default ctors/dctors
	VertexInitData() = default;
	~VertexInitData() = default;
};

/** Owns the vertex and index buffers of one mesh, along with the layout they were built for. */
class VertexFactory
{
	DISABLE_COPY(VertexFactory)

public:

	VertexFactory() = delete;		// The layout has to be given

	/**
	 * @param Type The layout the vertex data will be in
	 */
	VertexFactory(const VertexType Type);

	~VertexFactory() = default;

	/**
	 * Creates the vertex and index buffers and records their upload.
	 *
	 * @param Device Device the resources are created on
	 * @param CommandList List the copies and barriers are recorded into
	 * @param InitData Buffers to upload, moved in
	 * @return S_OK, failures throw out of the helpers
	 */
	template<typename VertexStructType>
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList, VertexInitData<VertexStructType> InitData)
	{
		// HRESULT, just in case...
		HRESULT hr = S_OK;
		// Store vertex type
		Type = InitData.Type;
		// Keep what picking needs before the buffers are handed over. The GPU copy can not be read back.
		CacheCpuGeometry(InitData.VertexData, InitData.IndexData);
		// Create and upload vertex buffer
		CreateAndUploadVertexBuffer(Device, CommandList,	std::move(InitData.VertexData));
		// Create and upload index buffer
		CreateAndUploadIndexBuffer(Device,	CommandList,	std::move(InitData.IndexData));

		// Return
		return hr;
	}

	ComPtr<DXResource>			VertexBufferUpload;	// Source of the copy, must outlive it
	ComPtr<DXResource>			VertexBuffer;		// Default heap, the one actually bound
	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView;	// Passed to IASetVertexBuffers
	VertexType					Type;				// Layout the buffers were built for

	/**
	 * @return How many vertices the vertex buffer holds. Only meaningful once Init has run.
	 */
	_NODISCARD UINT GetNumVertices() const noexcept
	{
		return VertexBufferView.StrideInBytes > 0
				? VertexBufferView.SizeInBytes / VertexBufferView.StrideInBytes
				: 0;
	}

	// Index Buffer
	ComPtr<DXResource>			IndexBufferUpload;
	ComPtr<DXResource>			IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW		IndexBufferView;

	// What a mouse click is tested against. The buffers above live on the GPU, so a ray can not
	// be cast against them, and what is kept is the positions and the triangle list only: 16
	// bytes a vertex of the layout come down to 12, and the rest is not needed to hit a surface.
	std::vector<DirectX::XMFLOAT3>	CpuPositions;
	std::vector<TypeOfIndice>		CpuIndices;

	// Box around the positions, in the mesh's own space. A cheap miss before the triangles are tried,
	// and the box the selection draws.
	DirectX::XMFLOAT3				LocalBoundsMin	= { 0.f, 0.f, 0.f };
	DirectX::XMFLOAT3				LocalBoundsMax	= { 0.f, 0.f, 0.f };

private:

	// Copies out the positions and indices and measures the box around them
	template<typename VertexStruct>
	void CacheCpuGeometry(const std::vector<VertexStruct>& VertexData, const std::vector<TypeOfIndice>& IndexData)
	{
		CpuPositions.clear();
		CpuPositions.reserve(VertexData.size());

		DirectX::XMFLOAT3 Min(FLT_MAX, FLT_MAX, FLT_MAX);
		DirectX::XMFLOAT3 Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (const auto& Vertex : VertexData)
		{
			const DirectX::XMFLOAT3 Position(Vertex.Position.x, Vertex.Position.y, Vertex.Position.z);
			CpuPositions.push_back(Position);

			Min = { std::min(Min.x, Position.x), std::min(Min.y, Position.y), std::min(Min.z, Position.z) };
			Max = { std::max(Max.x, Position.x), std::max(Max.y, Position.y), std::max(Max.z, Position.z) };
		}

		if (!CpuPositions.empty())
		{
			LocalBoundsMin	= Min;
			LocalBoundsMax	= Max;
		}

		CpuIndices = IndexData;
	}

	// Create + upload helpers (templated to accept std::vector with the Vertex/Index type)
	template<typename VertexStruct>
	void CreateAndUploadVertexBuffer(DXDevice* Device, DXGraphicsCommandList* CommandList, std::vector<VertexStruct>&& VertexData)
	{
		using Helpers::ThrowIfFailed;

		// Vertex type size in bytes (e.g. sizeof(VertexPositionNormal) = 32 bytes)
		const auto VertexTypeSize	= static_cast<UINT>(VertexTypeSizes[static_cast<uint32_t>(Type)]);
		// Vertex buffer size in bytes = number of vertices * size of each vertex (based on the vertex type)
		const auto VertexBufferSize	= static_cast<UINT>(VertexData.size() * VertexTypeSize);

		// Basic validation
		DXASSERT(Device != nullptr,												"Device can not be null");
		DXASSERT(CommandList != nullptr,										"CommandList can not be null");
		DXASSERT(VertexBufferSize > 0,											"Vertex buffer size must be >0");
		DXASSERT(VertexData.size() >= (VertexBufferSize / VertexTypeSize),		"VertexData does not contain enough elements");

		constexpr auto StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		constexpr auto StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

		// GPU-side vertex resource (default heap)
		Helpers::VERTEX_HELPER VertexGPU(
			Device,
			VertexBufferSize,
			DX_HEAP_PROPERTY_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON,
			L"VertexGPU");

		// Upload (intermediate) resource (upload heap)
		Helpers::VERTEX_HELPER VertexUploadToGPU(
			Device,
			VertexBufferSize,
			DX_HEAP_PROPERTY_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			L"VertexUploadToGPU");

		// Prepare subresource data pointing to the contiguous array memory
		D3D12_SUBRESOURCE_DATA VertexSubData = {};
		VertexSubData.pData      = reinterpret_cast<const void*>(VertexData.data());
		VertexSubData.RowPitch   = VertexBufferSize;
		VertexSubData.SlicePitch = VertexSubData.RowPitch;

		// Upload to GPU
		UpdateSubresources(CommandList, VertexGPU.GetPointer(), VertexUploadToGPU.GetPointer(), 0, 0, 1, &VertexSubData);

		// Transition to the vertex buffer state
		const auto VertexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexGPU.GetPointer(), StateBefore, StateAfter);
		CommandList->ResourceBarrier(1, &VertexBarrier);

		// Store views/resources on the factory
		VertexBufferView   = VertexGPU.CreateView(VertexTypeSize, VertexBufferSize);
		VertexBuffer       = VertexGPU.ReleaseResource();
		VertexBufferUpload = VertexUploadToGPU.ReleaseResource();
	}

	template<typename IndexType = TypeOfIndice>
	void CreateAndUploadIndexBuffer(DXDevice* Device, DXGraphicsCommandList* CommandList, std::vector<IndexType>&& IndexData)
	{
		using Helpers::ThrowIfFailed;

		// Size of each index (e.g. 4 bytes for uint32_t)
		const auto IndexTypeSize	= sizeof(IndexType);
		// Total size of index buffer in bytes
		const auto IndexBufferSize	= static_cast<UINT>(IndexData.size() * IndexTypeSize);

		// Basic validation
		DXASSERT(Device != nullptr,												"Device can not be null");
		DXASSERT(CommandList != nullptr,										"CommandList can not be null");
		DXASSERT(IndexBufferSize > 0,											"Index buffer size must be >0");
		DXASSERT(IndexData.size() >= (IndexBufferSize / IndexTypeSize),			"IndexData does not contain enough elements");

		constexpr auto StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		constexpr auto StateAfter  = D3D12_RESOURCE_STATE_INDEX_BUFFER;

		// GPU-side index resource (default heap)
		Helpers::INDEX_HELPER IndexGPU(
			Device,
			IndexBufferSize,
			DX_HEAP_PROPERTY_DEFAULT,
			D3D12_RESOURCE_STATE_COMMON,
			L"IndexGPU");

		// Upload (intermediate) resource (upload heap)
		Helpers::INDEX_HELPER IndexUploadToGPU(
			Device,
			IndexBufferSize,
			DX_HEAP_PROPERTY_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			L"IndexUploadToGPU");

		// Prepare subresource data pointing to the contiguous array memory
		D3D12_SUBRESOURCE_DATA IndexSubData = {};
		IndexSubData.pData      = reinterpret_cast<const void*>(IndexData.data());
		IndexSubData.RowPitch   = IndexBufferSize;
		IndexSubData.SlicePitch = IndexSubData.RowPitch;

		// Upload to GPU
		UpdateSubresources<1>(CommandList, IndexGPU.GetPointer(), IndexUploadToGPU.GetPointer(), 0, 0, 1, &IndexSubData);

		// Transition to the index buffer state
		const auto IndexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(IndexGPU.GetPointer(), StateBefore, StateAfter);
		CommandList->ResourceBarrier(1, &IndexBarrier);

		// Store views/resources on the factory
		IndexBufferView   = IndexGPU.CreateView(IndexBufferSize, DXGI_FORMAT_R32_UINT);
		IndexBuffer       = IndexGPU.ReleaseResource();
		IndexBufferUpload = IndexUploadToGPU.ReleaseResource();
	}
};

#endif // D3D12_VERTEX_H_INCLUDED
