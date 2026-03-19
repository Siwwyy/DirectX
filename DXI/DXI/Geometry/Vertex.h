//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_VERTEX_H_INCLUDED
#define D3D12_VERTEX_H_INCLUDED

#include "../pch.h"

/// <summary>
/// Enumeration of supported vertex attribute types.
/// Defines the composition of vertex data for different rendering scenarios.
/// </summary>
enum class VertexType : uint32_t
{
	/// Position only vertex (XMFLOAT4)
	Position,

	/// Position with normal vector (XMFLOAT4 + XMFLOAT4)
	PositionNormal,

	/// Position with color (XMFLOAT4 + XMFLOAT4)
	PositionColor,

	/// Position with color and normal (XMFLOAT4 + XMFLOAT4 + XMFLOAT4)
	PositionColorNormal,

	/// Position with texture coordinates (XMFLOAT4 + XMFLOAT2)
	PositionTexcoord,

	/// Position with texture coordinates and normal (XMFLOAT4 + XMFLOAT2 + XMFLOAT4)
	PositionTexcoordNormal,

	/// Total number of vertex type options
	VertexTypeCount
};

/// <summary>
/// Vertex layout structure containing only position data.
/// Matches VertexType::Position.
/// Total size: 16 bytes (4x FLOAT).
/// </summary>
struct VertexPosition
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// <summary>
	/// Deserializes vertex data from a byte buffer.
	/// Extracts position data from the provided byte array.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data.</param>
	/// <returns>VertexPosition structure populated with deserialized data.</returns>
	static VertexPosition DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		VertexPosition Vertex;
		DXASSERT(sizeof(DirectX::XMFLOAT4) <= Data.size(), L"Insufficient buffer size for VertexPosition deserialization");
		std::memcpy(&Vertex.Position,	&Data[0],	sizeof(DirectX::XMFLOAT4));
		return Vertex;
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPosition));
		std::memcpy(&Data[0],	&Position,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/// <summary>
/// Vertex layout structure containing position and normal data.
/// Matches VertexType::PositionNormal.
/// Total size: 32 bytes (4x FLOAT + 4x FLOAT).
/// </summary>
struct VertexPositionNormal
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// Surface normal vector (W component unused)
	DirectX::XMFLOAT4 Normal;

	/// <summary>
	/// Deserializes vertex data from a byte buffer.
	/// Extracts position and normal data from the provided byte array.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data.</param>
	/// <returns>VertexPositionNormal structure populated with deserialized data.</returns>
	static VertexPositionNormal DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		VertexPositionNormal Vertex;
		DXASSERT(sizeof(VertexPositionNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionNormal deserialization");
		std::memcpy(&Vertex.Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Vertex.Normal,		&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
		return Vertex;
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position and normal data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionNormal));
		std::memcpy(&Data[0],							&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],	&Normal,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/// <summary>
/// Vertex layout structure containing position and color data.
/// Matches VertexType::PositionColor.
/// Total size: 32 bytes (4x FLOAT + 4x FLOAT).
/// </summary>
struct VertexPositionColor
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// RGBA color information
	DirectX::XMFLOAT4 Color;

	/// <summary>
	/// Deserializes vertex data from a byte buffer.
	/// Extracts position and color data from the provided byte array.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data.</param>
	/// <returns>VertexPositionColor structure populated with deserialized data.</returns>
	static VertexPositionColor DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		VertexPositionColor Vertex;
		DXASSERT(sizeof(VertexPositionColor) <= Data.size(), L"Insufficient buffer size for VertexPositionColor deserialization");
		std::memcpy(&Vertex.Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Vertex.Color,		&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
		return Vertex;
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position and color data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionColor));
		std::memcpy(&Data[0],							&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],	&Color,		sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/// <summary>
/// Vertex layout structure containing position, color, and normal data.
/// Matches VertexType::PositionColorNormal.
/// Total size: 48 bytes (4x FLOAT + 4x FLOAT + 4x FLOAT).
/// </summary>
struct VertexPositionColorNormal
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// RGBA color information
	DirectX::XMFLOAT4 Color;

	/// Surface normal vector (W component unused)
	DirectX::XMFLOAT4 Normal;

	/// <summary>
	/// Deserializes vertex data from a byte buffer.
	/// Extracts position, color, and normal data from the provided byte array.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data.</param>
	/// <returns>VertexPositionColorNormal structure populated with deserialized data.</returns>
	static VertexPositionColorNormal DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		VertexPositionColorNormal Vertex;
		DXASSERT(sizeof(VertexPositionColorNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionColorNormal deserialization");
		std::memcpy(&Vertex.Position,	&Data[0],														sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Vertex.Color,		&Data[sizeof(DirectX::XMFLOAT4)],								sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Vertex.Normal,		&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
		return Vertex;
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position, color, and normal data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionColorNormal));
		std::memcpy(&Data[0],														&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],								&Color,		sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT4)],	&Normal,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/// <summary>
/// Vertex layout structure containing position and texture coordinate data.
/// Matches VertexType::PositionTexcoord.
/// Total size: 24 bytes (4x FLOAT + 2x FLOAT).
/// </summary>
struct VertexPositionTexcoord
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// UV texture coordinates
	DirectX::XMFLOAT2 TexCoord;

	/// <summary>
	/// Deserializes vertex data from a byte buffer.
	/// Extracts position and texture coordinate data from the provided byte array.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data.</param>
	/// <returns>VertexPositionTexcoord structure populated with deserialized data.</returns>
	static VertexPositionTexcoord DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		VertexPositionTexcoord Vertex;
		DXASSERT(sizeof(VertexPositionTexcoord) <= Data.size(), L"Insufficient buffer size for VertexPositionTexcoord deserialization");
		std::memcpy(&Vertex.Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Vertex.TexCoord,	&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT2));
		return Vertex;
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position and texture coordinate data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionTexcoord));
		std::memcpy(&Data[0],							&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],	&TexCoord,	sizeof(DirectX::XMFLOAT2));
		return Data;
	}
};

/// <summary>
/// Vertex layout structure containing position, texture coordinate, and normal data.
/// Matches VertexType::PositionTexcoordNormal.
/// Total size: 40 bytes (4x FLOAT + 2x FLOAT + 4x FLOAT).
/// </summary>
struct VertexPositionTexcoordNormal
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// UV texture coordinates
	DirectX::XMFLOAT2 TexCoord;

	/// Surface normal vector (W component unused)
	DirectX::XMFLOAT4 Normal;

	/// <summary>
	/// Deserializes vertex data from a byte buffer.
	/// Extracts position, texture coordinate, and normal data from the provided byte array.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data.</param>
	/// <returns>VertexPositionTexcoordNormal structure populated with deserialized data.</returns>
	static VertexPositionTexcoordNormal DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		VertexPositionTexcoordNormal Vertex;
		DXASSERT(sizeof(VertexPositionTexcoordNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionTexcoordNormal deserialization");
		std::memcpy(&Vertex.Position,	&Data[0],														sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Vertex.TexCoord,	&Data[sizeof(DirectX::XMFLOAT4)],								sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Vertex.Normal,		&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],	sizeof(DirectX::XMFLOAT4));
		return Vertex;
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position, texture coordinate, and normal data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionTexcoordNormal));
		std::memcpy(&Data[0],														&Position,	sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4)],								&TexCoord,	sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],	&Normal,	sizeof(DirectX::XMFLOAT4));
		return Data;
	}
};

/// <summary>
/// Array mapping VertexType enumeration values to their corresponding vertex structure sizes.
/// Used for memory allocation and validation during vertex buffer operations.
/// </summary>
const uint64_t VertexTypeSizes[] = {
	sizeof(VertexPosition),					/// VertexType::Position
	sizeof(VertexPositionNormal),			/// VertexType::PositionNormal
	sizeof(VertexPositionColor),			/// VertexType::PositionColor
	sizeof(VertexPositionColorNormal),		/// VertexType::PositionColorNormal
	sizeof(VertexPositionTexcoord),			/// VertexType::PositionTexcoord
	sizeof(VertexPositionTexcoordNormal)	/// VertexType::PositionTexcoordNormal
};

/// <summary>
/// Vertex descriptor class for managing vertex type information.
/// Tracks the type of vertex attributes used in rendering operations.
/// </summary>
class Vertex
{
	DISABLE_COPY(Vertex)

public:

	/// <summary>
	/// Default constructor deleted - Vertex type must be specified.
	/// </summary>
	Vertex() = delete;

	/// <summary>
	/// Constructor that initializes a Vertex with a specific type.
	/// </summary>
	/// <param name="Type">The VertexType that defines the vertex attribute composition.</param>
	Vertex(const VertexType Type);

	/// <summary>
	/// Default destructor.
	/// </summary>
	~Vertex() = default;

	/// <summary>
	/// Initializes the vertex with device and command list resources.
	/// Sets up necessary GPU resources for vertex buffer operations.
	/// </summary>
	/// <param name="Device">Pointer to the Direct3D device.</param>
	/// <param name="CommandList">Pointer to the graphics command list.</param>
	/// <returns>HRESULT indicating success or failure of initialization.</returns>
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList);

private:

	/// <summary>
	/// The type of vertex attributes this vertex descriptor represents.
	/// Determines the layout and composition of vertex data.
	/// </summary>
	VertexType Type;

	/// <summary>
	/// Raw vertex data buffer.
	/// Size and format depend on the VertexType value.
	/// </summary>
	std::vector<BYTE> VertexData;
};


#endif // D3D12_VERTEX_H_INCLUDED