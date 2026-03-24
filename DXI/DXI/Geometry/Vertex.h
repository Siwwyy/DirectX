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

	/// ctor: position (x,y,z[,pw])
	VertexPosition(float x = 0.0f, float y = 0.0f, float z = 0.0f, float pw = 1.0f) noexcept
		: Position(x, y, z, pw)
	{
	}

	/// <summary>
	/// Populate this structure from a byte buffer.
	/// Copies the position element from the provided contiguous byte array into this instance.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data. Must contain at least sizeof(DirectX::XMFLOAT4) bytes.</param>
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(DirectX::XMFLOAT4) <= Data.size(), L"Insufficient buffer size for VertexPosition deserialization");
		std::memcpy(&Position,	&Data[0],	sizeof(DirectX::XMFLOAT4));
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

	/// ctor: position (x,y,z[,pw]) and normal (nx,ny,nz[,nw])
	VertexPositionNormal(float x = 0.0f, float y = 0.0f, float z = 0.0f,
						float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
						float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, Normal(nx, ny, nz, nw)
	{
	}

	/// <summary>
	/// Populate this structure from a byte buffer.
	/// Copies position and normal elements from the provided contiguous byte array into this instance.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data. Must contain at least sizeof(VertexPositionNormal) bytes.</param>
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionNormal deserialization");
		std::memcpy(&Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
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

	/// ctor: position (x,y,z[,pw]) and color (r,g,b,a)
	VertexPositionColor(float x = 0.0f, float y = 0.0f, float z = 0.0f,
						float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
						float pw = 1.0f) noexcept
		: Position(x, y, z, pw)
		, Color(r, g, b, a)
	{
	}

	/// <summary>
	/// Populate this structure from a byte buffer.
	/// Copies position and color elements from the provided contiguous byte array into this instance.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data. Must contain at least sizeof(VertexPositionColor) bytes.</param>
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionColor) <= Data.size(), L"Insufficient buffer size for VertexPositionColor deserialization");
		std::memcpy(&Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Color,		&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
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

	/// ctor: position (x,y,z[,pw]), color (r,g,b,a), normal (nx,ny,nz[,nw])
	VertexPositionColorNormal(float x = 0.0f, float y = 0.0f, float z = 0.0f,
								float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
								float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
								float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, Color(r, g, b, a)
		, Normal(nx, ny, nz, nw)
	{
	}

	/// <summary>
	/// Populate this structure from a byte buffer.
	/// Copies position, color and normal elements from the provided contiguous byte array into this instance.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data. Must contain at least sizeof(VertexPositionColorNormal) bytes.</param>
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionColorNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionColorNormal deserialization");
		std::memcpy(&Position,	&Data[0],														sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Color,		&Data[sizeof(DirectX::XMFLOAT4)],								sizeof(DirectX::XMFLOAT4));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT4));
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
struct VertexPositionTexCoord
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// UV texture coordinates
	DirectX::XMFLOAT2 TexCoord;

	/// ctor: position (x,y,z[,pw]) and texcoord (u,v)
	VertexPositionTexCoord(float x = 0.0f, float y = 0.0f, float z = 0.0f,
							float u = 0.0f, float v = 0.0f,
							float pw = 1.0f) noexcept
		: Position(x, y, z, pw)
		, TexCoord(u, v)
	{
	}

	/// <summary>
	/// Populate this structure from a byte buffer.
	/// Copies position and texcoord elements from the provided contiguous byte array into this instance.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data. Must contain at least sizeof(VertexPositionTexCoord) bytes.</param>
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionTexCoord) <= Data.size(), L"Insufficient buffer size for VertexPositionTexCoord deserialization");
		std::memcpy(&Position,	&Data[0],							sizeof(DirectX::XMFLOAT4));
		std::memcpy(&TexCoord,	&Data[sizeof(DirectX::XMFLOAT4)],	sizeof(DirectX::XMFLOAT2));
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position and texture coordinate data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionTexCoord));
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
struct VertexPositionTexCoordNormal
{
	/// 3D position (W component typically set to 1.0f)
	DirectX::XMFLOAT4 Position;

	/// UV texture coordinates
	DirectX::XMFLOAT2 TexCoord;

	/// Surface normal vector (W component unused)
	DirectX::XMFLOAT4 Normal;

	/// ctor: position (x,y,z[,pw]), texcoord (u,v), normal (nx,ny,nz[,nw])
	VertexPositionTexCoordNormal(float x = 0.0f, float y = 0.0f, float z = 0.0f,
								float u = 0.0f, float v = 0.0f,
								float nx = 0.0f, float ny = 0.0f, float nz = 1.0f,
								float pw = 1.0f, float nw = 0.0f) noexcept
		: Position(x, y, z, pw)
		, TexCoord(u, v)
		, Normal(nx, ny, nz, nw)
	{
	}

	/// <summary>
	/// Populate this structure from a byte buffer.
	/// Copies position, texcoord and normal elements from the provided contiguous byte array into this instance.
	/// </summary>
	/// <param name="Data">Vector containing serialized vertex data. Must contain at least sizeof(VertexPositionTexCoordNormal) bytes.</param>
	void DeserializeFromBytes(const std::vector<BYTE>& Data)
	{
		DXASSERT(sizeof(VertexPositionTexCoordNormal) <= Data.size(), L"Insufficient buffer size for VertexPositionTexCoordNormal deserialization");
		std::memcpy(&Position,	&Data[0],														sizeof(DirectX::XMFLOAT4));
		std::memcpy(&TexCoord,	&Data[sizeof(DirectX::XMFLOAT4)],								sizeof(DirectX::XMFLOAT2));
		std::memcpy(&Normal,	&Data[sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT2)],	sizeof(DirectX::XMFLOAT4));
	}

	/// <summary>
	/// Serializes vertex data to a byte buffer.
	/// Converts position, texture coordinate, and normal data into a byte array for storage or transmission.
	/// </summary>
	/// <returns>Vector containing serialized vertex data.</returns>
	std::vector<BYTE> SerializeToBytes() const
	{
		std::vector<BYTE> Data(sizeof(VertexPositionTexCoordNormal));
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
	sizeof(VertexPositionTexCoord),			/// VertexType::PositionTexcoord
	sizeof(VertexPositionTexCoordNormal)	/// VertexType::PositionTexcoordNormal
};

template<typename In, typename Out, size_t N, size_t M>
std::array<Out, N> ComputeFaceNormal(const In (&VertexArray)[N], const DWORD (&IndicesList)[M])
{
	constexpr size_t NumVertices = N;	// Number of vertices in the input array
	constexpr size_t NumVerticesPerTriangle = 3;	// Number of vertices in triangle (everytime its 3, but who knows?)
	constexpr size_t NumTriangles = M / NumVerticesPerTriangle; // Assuming each triangle is defined by 3 indices
	std::array<Out, N> out{};

	for (size_t i = 0; i < NumTriangles; ++i)
	{
		// Get values
		const DWORD IdxX0	= IndicesList[i * NumVerticesPerTriangle + 0];
		const DWORD IdxX1	= IndicesList[i * NumVerticesPerTriangle + 1];
		const DWORD IdxX2	= IndicesList[i * NumVerticesPerTriangle + 2];
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
		XMVECTOR UnitVector = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		bool IsNormalUnitLength = XMVector3NearEqual(XMVector3Length(Normal), XMVector3Length(UnitVector), Epsilon);
		DXASSERT(IsNormalUnitLength, "Normal vector should be unit length vector");

		// Serialize to bytes
		std::vector<BYTE> DataVertex0 = Vertex0.SerializeToBytes();
		std::vector<BYTE> DataVertex1 = Vertex1.SerializeToBytes();
		std::vector<BYTE> DataVertex2 = Vertex2.SerializeToBytes();

		// Create NormalXYZW for memcpy
		const float NormalXYZW[] = { XMVectorGetX(Normal), XMVectorGetY(Normal), XMVectorGetZ(Normal), 0.0f };
		const size_t NormalSize = sizeof(NormalXYZW);

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

/// <summary>
/// VertexInitData
/// 
/// Encapsulates data required to initialize a `Vertex` instance.
/// This small value-type aggregates:
/// - `Type` : the `VertexType` identifying the layout/format of the data in `VertexData`.
/// - `VertexBufferSize` : the number of bytes contained in `VertexData`. When using the convenience
///   constructor this is automatically synchronized with `VertexData.size()`.
/// - `VertexData` : an owned contiguous byte buffer containing raw vertex bytes ready to be uploaded
///   to the GPU.
/// 
/// Design notes:
/// - Copy semantics are intentionally disabled via `DISABLE_COPY`; use move semantics to transfer
///   ownership of `VertexData` into `Vertex::Init(...)` without copying large buffers.
/// - The struct is default-constructible so callers can populate members explicitly; when possible,
///   prefer constructing with a ready buffer and moving it into `VertexInitData`.
/// 
/// Example:
/// `auto init = Vertex::VertexInitData{ VertexType::Position, std::move(myByteBuffer) };`
/// `vertex.Init(device, cmdList, std::move(init));`
/// </summary>
struct VertexInitData
{
	DISABLE_COPY(VertexInitData)

	// Make fields movable by removing const qualifiers.
	// VertexBufferSize is kept in sync with VertexData.size() by helper ctor.
	VertexType Type{ VertexType::Position };
	size_t VertexBufferSize{ 0 };
	std::vector<BYTE> VertexData;

	// Default ctors/dctors
	VertexInitData() noexcept = default;
	~VertexInitData() = default;
};

/// <summary>
/// Vertex descriptor class for managing vertex type information.
/// Tracks the type of vertex attributes used in rendering operations.
/// </summary>
class VertexFactory
{
	DISABLE_COPY(VertexFactory)

public:

	/// <summary>
	/// Default constructor deleted - Vertex type must be specified.
	/// </summary>
	VertexFactory() = delete;

	/// <summary>
	/// Constructor that initializes a Vertex with a specific type.
	/// </summary>
	/// <param name="Type">The VertexType that defines the vertex attribute composition.</param>
	VertexFactory(const VertexType Type);

	/// <summary>
	/// Default destructor.
	/// </summary>
	~VertexFactory() = default;

	/// <summary>
	/// Initializes the vertex with device and command list resources.
	/// Sets up necessary GPU resources for vertex buffer operations.
	/// </summary>
	/// <param name="Device">Pointer to the Direct3D device.</param>
	/// <param name="CommandList">Pointer to the graphics command list.</param>
	/// <param name="VertexInitData">Vertex Initial Data, struct to fill and std::move().</param>
	/// <returns>HRESULT indicating success or failure of initialization.</returns>
	HRESULT Init(DXDevice* Device, DXGraphicsCommandList * CommandList, VertexInitData InitData);

	/// <summary>
	/// GPU-side upload resource (upload heap) that temporarily holds the vertex bytes on the CPU.
	/// This resource is used as the source for copy/update operations (e.g. UpdateSubresources)
	/// to transfer vertex data into the GPU-resident default-heap resource. Keep this alive
	/// until the GPU has finished executing the command list that performed the copy.
	/// </summary>
	ComPtr<DXResource>			VertexBufferUpload;

	/// <summary>
	/// GPU-resident vertex buffer resource (default heap). After uploading data from
	/// `VertexBufferUpload` this resource is the buffer actually bound for rendering.
	/// Typically created with D3D12_HEAP_TYPE_DEFAULT and used for GPU reads only.
	/// </summary>
	ComPtr<DXResource>			VertexBuffer;

	/// <summary>
	/// Describes the vertex buffer for the input assembler:
	/// - BufferLocation: GPU virtual address of the vertex buffer start.
	/// - SizeInBytes: Total size of the buffer (in bytes).
	/// - StrideInBytes: Size of a single vertex element (in bytes).
	/// Populate this view after creating/filling `VertexBuffer` and pass it to
	/// `IASetVertexBuffers` prior to draw calls.
	/// </summary>
	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView;

	///// <summary>
	///// The type of vertex attributes this vertex descriptor represents.
	///// Determines the layout and composition of vertex data.
	///// </summary>
	VertexType Type;

private:
};

#endif // D3D12_VERTEX_H_INCLUDED