#include "GltfLoader.h"

#include <cfloat>
#include <filesystem>
#include <stdexcept>
#include <utility>

// cgltf is a single header, so exactly one translation unit has to pull in its bodies and
// this is it. The warnings come out of the library rather than our use of it: it is C, so
// it reaches for fopen and friends that MSVC marks deprecated, and it narrows sizes freely.
#pragma warning(push)
#pragma warning(disable : 4996)		// 'fopen': This function or variable may be unsafe
#pragma warning(disable : 4267)		// conversion from 'size_t' to a smaller type
#pragma warning(disable : 4244)		// conversion from 'cgltf_float' to a smaller type
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#pragma warning(pop)

using namespace DirectX;

using TypeOfVertex = VertexPositionTexCoordNormalTangentBitangent;

namespace
{
	// cgltf speaks UTF-8 char paths, the rest of the engine speaks wide ones
	_NODISCARD std::string ToUtf8(const std::wstring& Wide)
	{
		if (Wide.empty())
		{
			return std::string();
		}

		const int Length = WideCharToMultiByte(CP_UTF8, 0, Wide.c_str(), static_cast<int>(Wide.size()), nullptr, 0, nullptr, nullptr);
		DXASSERT(Length > 0, "Path could not be converted to UTF-8");

		std::string Narrow(static_cast<size_t>(Length), '\0');
		WideCharToMultiByte(CP_UTF8, 0, Wide.c_str(), static_cast<int>(Wide.size()), &Narrow[0], Length, nullptr, nullptr);
		return Narrow;
	}

	_NODISCARD std::wstring FromUtf8(const char* Narrow)
	{
		if (Narrow == nullptr || *Narrow == '\0')
		{
			return std::wstring();
		}

		const int Length = MultiByteToWideChar(CP_UTF8, 0, Narrow, -1, nullptr, 0);
		if (Length <= 1)
		{
			return std::wstring();
		}

		std::wstring Wide(static_cast<size_t>(Length - 1), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, Narrow, -1, &Wide[0], Length);
		return Wide;
	}

	// Turns a glTF image reference into something that can be opened.
	//
	// A uri is relative to the file that named it and may be percent encoded, since it is a
	// uri rather than a path. It can also be absent entirely: a .glb keeps its images in the
	// binary blob, and a .gltf may inline them as a data: uri. Neither can be handed to a
	// texture loader that only knows how to open files, so both come back empty.
	_NODISCARD std::wstring ResolveImagePath(const std::filesystem::path& BaseDir, const cgltf_image* Image)
	{
		if (Image == nullptr || Image->uri == nullptr)
		{
			return std::wstring();
		}

		if (strncmp(Image->uri, "data:", 5) == 0)
		{
			return std::wstring();
		}

		std::string Uri(Image->uri);
		cgltf_decode_uri(&Uri[0]);
		Uri.resize(strlen(Uri.c_str()));		// decode_uri shortens in place, the string has to catch up

		const std::filesystem::path Resolved = BaseDir / std::filesystem::path(Uri);
		return Resolved.wstring();
	}

	_NODISCARD std::wstring ResolveTexturePath(const std::filesystem::path& BaseDir, const cgltf_texture_view& View)
	{
		if (View.texture == nullptr)
		{
			return std::wstring();
		}

		return ResolveImagePath(BaseDir, View.texture->image);
	}

	// The accessor behind one attribute, or null when the primitive does not carry it.
	// TexCoordIdx picks between TEXCOORD_0, TEXCOORD_1 and so on; we only ever read the first.
	_NODISCARD const cgltf_accessor* FindAttribute(const cgltf_primitive& Primitive, const cgltf_attribute_type Type, const int AttributeIdx = 0)
	{
		for (cgltf_size Idx = 0; Idx < Primitive.attributes_count; ++Idx)
		{
			const cgltf_attribute& Attribute = Primitive.attributes[Idx];

			if (Attribute.type == Type && Attribute.index == AttributeIdx)
			{
				return Attribute.data;
			}
		}

		return nullptr;
	}

	// Fills in normals the file did not supply, by averaging the planes of the triangles
	// that meet at each vertex. Smooth rather than flat, which is the right default for a
	// welded mesh: the vertices are shared precisely because the artist wanted them shared.
	void GenerateNormals(LoadedMesh& Mesh)
	{
		for (TypeOfVertex& Vertex : Mesh.Vertices)
		{
			Vertex.Normal = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
		}

		for (size_t Idx = 0; Idx + 2 < Mesh.Indices.size(); Idx += 3)
		{
			TypeOfVertex& Vertex0 = Mesh.Vertices[Mesh.Indices[Idx + 0]];
			TypeOfVertex& Vertex1 = Mesh.Vertices[Mesh.Indices[Idx + 1]];
			TypeOfVertex& Vertex2 = Mesh.Vertices[Mesh.Indices[Idx + 2]];

			const XMVECTOR Position0 = XMLoadFloat4(&Vertex0.Position);
			const XMVECTOR Edge1 = XMVectorSubtract(XMLoadFloat4(&Vertex1.Position), Position0);
			const XMVECTOR Edge2 = XMVectorSubtract(XMLoadFloat4(&Vertex2.Position), Position0);

			// Not normalized on purpose: the cross product's length is twice the triangle's
			// area, so a big triangle gets the say over the corner it shares with a sliver
			const XMVECTOR FaceNormal = XMVector3Cross(Edge2, Edge1);

			for (TypeOfVertex* Vertex : { &Vertex0, &Vertex1, &Vertex2 })
			{
				XMStoreFloat4(&Vertex->Normal, XMVectorAdd(XMLoadFloat4(&Vertex->Normal), FaceNormal));
			}
		}

		for (TypeOfVertex& Vertex : Mesh.Vertices)
		{
			XMStoreFloat4(&Vertex.Normal, XMVector3Normalize(XMLoadFloat4(&Vertex.Normal)));
			Vertex.Normal.w = 0.f;
		}
	}

	// Fills in tangents the file did not supply, by the same averaging as the normals.
	// Left unnormalized and off the surface, since OrthonormalizeFrames runs over every
	// mesh afterwards whether its tangents were generated or read.
	void GenerateTangents(LoadedMesh& Mesh)
	{
		for (TypeOfVertex& Vertex : Mesh.Vertices)
		{
			Vertex.Tangent = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
		}

		for (size_t Idx = 0; Idx + 2 < Mesh.Indices.size(); Idx += 3)
		{
			TypeOfVertex& Vertex0 = Mesh.Vertices[Mesh.Indices[Idx + 0]];
			TypeOfVertex& Vertex1 = Mesh.Vertices[Mesh.Indices[Idx + 1]];
			TypeOfVertex& Vertex2 = Mesh.Vertices[Mesh.Indices[Idx + 2]];

			const TriangleFrame Frame = ComputeTriangleFrame(
				XMFLOAT3(Vertex0.Position.x, Vertex0.Position.y, Vertex0.Position.z),
				XMFLOAT3(Vertex1.Position.x, Vertex1.Position.y, Vertex1.Position.z),
				XMFLOAT3(Vertex2.Position.x, Vertex2.Position.y, Vertex2.Position.z),
				Vertex0.TexCoord, Vertex1.TexCoord, Vertex2.TexCoord);

			const XMVECTOR FaceTangent = XMLoadFloat4(&Frame.Tangent);

			for (TypeOfVertex* Vertex : { &Vertex0, &Vertex1, &Vertex2 })
			{
				const XMVECTOR Accumulated = XMVectorAdd(XMLoadFloat4(&Vertex->Tangent), FaceTangent);
				XMStoreFloat4(&Vertex->Tangent, Accumulated);
				Vertex->Tangent.w = Frame.Tangent.w;		// Handedness is not an average, the last triangle wins
			}
		}
	}

	// Forces the normal and the tangent to be unit length and perpendicular.
	//
	// Runs over every mesh, not only the ones whose tangents were generated. A file is
	// entitled to supply a tangent that is a degree or two off its normal, and exporters
	// routinely do, which leaves the basis very slightly sheared. Sampling a normal map
	// through a sheared basis tilts the result, and the error moves as the surface curves,
	// which is exactly the kind of fault that shows up as a smear in a reflection and is
	// almost impossible to place later. Sponza arrives with 219 such vertices.
	//
	// Gram-Schmidt: drop whatever part of the tangent points along the normal, and keep
	// what is left. The direction the tangent was mostly pointing is preserved, which is
	// what matters, since the uv shell is what chose it.
	void OrthonormalizeFrames(LoadedMesh& Mesh)
	{
		for (TypeOfVertex& Vertex : Mesh.Vertices)
		{
			const XMVECTOR Normal	= XMVector3Normalize(XMLoadFloat4(&Vertex.Normal));
			XMVECTOR Tangent		= XMLoadFloat4(&Vertex.Tangent);

			Tangent = XMVectorSubtract(Tangent, XMVectorMultiply(Normal, XMVector3Dot(Normal, Tangent)));

			// Collapsed onto the normal, so the uv gave no usable direction at all. Any
			// perpendicular will do: a normal map read through it is wrong, but a basis
			// that is merely rotated beats one that is degenerate.
			if (XMVectorGetX(XMVector3LengthSq(Tangent)) < 1e-12f)
			{
				const XMVECTOR Axis = fabsf(XMVectorGetX(Normal)) < 0.9f ? XMVectorSet(1.f, 0.f, 0.f, 0.f) : XMVectorSet(0.f, 1.f, 0.f, 0.f);
				Tangent = XMVector3Cross(Normal, Axis);
			}

			const float Handedness = Vertex.Tangent.w != 0.f ? Vertex.Tangent.w : 1.f;

			XMStoreFloat4(&Vertex.Normal, Normal);
			Vertex.Normal.w = 0.f;

			XMStoreFloat4(&Vertex.Tangent, XMVector3Normalize(Tangent));
			Vertex.Tangent.w = Handedness;
		}
	}

	// The third axis of the basis, which our layout stores outright rather than deriving in
	// the pixel shader. Runs after the normal and the tangent are final, whether they came
	// out of the file or were generated above.
	void BuildBitangents(LoadedMesh& Mesh)
	{
		for (TypeOfVertex& Vertex : Mesh.Vertices)
		{
			const XMVECTOR Normal		= XMLoadFloat4(&Vertex.Normal);
			const XMVECTOR Tangent		= XMLoadFloat4(&Vertex.Tangent);
			const XMVECTOR Bitangent	= XMVectorScale(XMVector3Cross(Normal, Tangent), Vertex.Tangent.w);

			XMStoreFloat4(&Vertex.Bitangent, XMVector3Normalize(Bitangent));
			Vertex.Bitangent.w = 0.f;
		}
	}

	// Reads one primitive into our vertex layout, in world space and left handed.
	//
	// World is the node's world transform with the Z mirror already folded in, so a position
	// only has to be pushed through one matrix. NormalMatrix is its inverse transpose, which
	// is what keeps a normal perpendicular to a surface that has been scaled unevenly.
	_NODISCARD LoadedMesh ReadPrimitive(const cgltf_primitive& Primitive, const XMMATRIX& World, const XMMATRIX& NormalMatrix)
	{
		const cgltf_accessor* PositionAccessor	= FindAttribute(Primitive, cgltf_attribute_type_position);
		const cgltf_accessor* NormalAccessor	= FindAttribute(Primitive, cgltf_attribute_type_normal);
		const cgltf_accessor* TangentAccessor	= FindAttribute(Primitive, cgltf_attribute_type_tangent);
		const cgltf_accessor* TexCoordAccessor	= FindAttribute(Primitive, cgltf_attribute_type_texcoord);

		LoadedMesh Mesh;

		if (PositionAccessor == nullptr || PositionAccessor->count == 0)
		{
			return Mesh;		// Nothing to draw, the caller drops it
		}

		const size_t VertexCount = static_cast<size_t>(PositionAccessor->count);
		Mesh.Vertices.resize(VertexCount);

		for (size_t Idx = 0; Idx < VertexCount; ++Idx)
		{
			TypeOfVertex& Vertex = Mesh.Vertices[Idx];

			float Position[3] = { 0.f, 0.f, 0.f };
			cgltf_accessor_read_float(PositionAccessor, Idx, Position, 3);
			XMStoreFloat4(&Vertex.Position, XMVector3Transform(XMVectorSet(Position[0], Position[1], Position[2], 1.f), World));
			Vertex.Position.w = 1.f;

			if (TexCoordAccessor != nullptr)
			{
				float TexCoord[2] = { 0.f, 0.f };
				cgltf_accessor_read_float(TexCoordAccessor, Idx, TexCoord, 2);

				// No V flip. glTF puts the uv origin at the top left, which is where D3D
				// samples from as well, unlike OBJ where the origin is at the bottom
				Vertex.TexCoord = XMFLOAT2(TexCoord[0], TexCoord[1]);
			}

			if (NormalAccessor != nullptr)
			{
				float Normal[3] = { 0.f, 0.f, 1.f };
				cgltf_accessor_read_float(NormalAccessor, Idx, Normal, 3);
				XMStoreFloat4(&Vertex.Normal, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(Normal[0], Normal[1], Normal[2], 0.f), NormalMatrix)));
				Vertex.Normal.w = 0.f;
			}

			if (TangentAccessor != nullptr)
			{
				// A glTF tangent is a float4: xyz along growing U, w the handedness the
				// bitangent is scaled by. Our layout agrees, which is the one piece of this
				// that needs no translation beyond the mirror
				float Tangent[4] = { 1.f, 0.f, 0.f, 1.f };
				cgltf_accessor_read_float(TangentAccessor, Idx, Tangent, 4);
				XMStoreFloat4(&Vertex.Tangent, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(Tangent[0], Tangent[1], Tangent[2], 0.f), World)));

				// Mirroring flips the sign of every cross product, so the handedness that
				// described this uv shell in glTF's space describes the opposite in ours
				Vertex.Tangent.w = -Tangent[3];
			}
		}

		if (Primitive.indices != nullptr)
		{
			const size_t IndexCount = static_cast<size_t>(Primitive.indices->count);
			Mesh.Indices.resize(IndexCount);

			static_assert(sizeof(TypeOfIndice) == 4, "cgltf is being asked to unpack into 4 byte indices");
			cgltf_accessor_unpack_indices(Primitive.indices, Mesh.Indices.data(), sizeof(TypeOfIndice), IndexCount);
		}
		else
		{
			// A primitive may leave its indices out, in which case the vertices are already
			// in triangle order
			Mesh.Indices.resize(VertexCount);
			for (size_t Idx = 0; Idx < VertexCount; ++Idx)
			{
				Mesh.Indices[Idx] = static_cast<TypeOfIndice>(Idx);
			}
		}

		// Mirroring Z turns a front face into a back face, because reflecting the geometry
		// reverses the order its corners are met in without touching the order they are
		// listed in. Swapping two corners of every triangle puts that back.
		for (size_t Idx = 0; Idx + 2 < Mesh.Indices.size(); Idx += 3)
		{
			std::swap(Mesh.Indices[Idx + 1], Mesh.Indices[Idx + 2]);
		}

		if (NormalAccessor == nullptr)
		{
			GenerateNormals(Mesh);
		}

		if (TangentAccessor == nullptr)
		{
			GenerateTangents(Mesh);
		}

		OrthonormalizeFrames(Mesh);
		BuildBitangents(Mesh);

		return Mesh;
	}

	// Walks the node tree, emitting one GltfPrimitive per mesh primitive found on the way.
	//
	// Recursive because glTF nests, but cgltf has already resolved each node's world
	// transform from its parents, so the recursion only has to find the nodes rather than
	// carry anything down.
	void CollectNode(const cgltf_data& Data, const cgltf_node& Node, const XMMATRIX& MirrorZ, GltfScene& OutScene)
	{
		if (Node.mesh != nullptr)
		{
			// Column major in the file, and glTF multiplies a matrix onto a column vector
			// where DirectXMath multiplies a row vector onto a matrix. Reading the array
			// straight into rows performs the transpose those two conventions differ by.
			float NodeMatrix[16] = {};
			cgltf_node_transform_world(&Node, NodeMatrix);

			const XMMATRIX World		= XMMatrixMultiply(XMMATRIX(NodeMatrix), MirrorZ);
			XMVECTOR Determinant		= XMVectorZero();
			const XMMATRIX NormalMatrix	= XMMatrixTranspose(XMMatrixInverse(&Determinant, World));

			const std::wstring MeshName = Node.mesh->name != nullptr ? FromUtf8(Node.mesh->name) : L"Mesh";

			for (cgltf_size Idx = 0; Idx < Node.mesh->primitives_count; ++Idx)
			{
				const cgltf_primitive& Primitive = Node.mesh->primitives[Idx];

				// Everything that is not a triangle list is skipped rather than converted.
				// Strips, fans and point clouds all appear in the format and none of them
				// survive the index rewriting above.
				if (Primitive.type != cgltf_primitive_type_triangles)
				{
					DXLOG("glTF: skipping primitive %u, only triangle lists are supported\n", static_cast<UINT>(Idx))
					continue;
				}

				GltfPrimitive Out;
				Out.Mesh = ReadPrimitive(Primitive, World, NormalMatrix);

				if (Out.Mesh.Vertices.empty() || Out.Mesh.Indices.empty())
				{
					continue;
				}

				Out.Name		= MeshName + L"_" + std::to_wstring(Idx);
				Out.MaterialIdx	= Primitive.material != nullptr
									? static_cast<int>(Primitive.material - Data.materials)
									: -1;

				OutScene.Primitives.push_back(std::move(Out));
			}
		}

		for (cgltf_size Idx = 0; Idx < Node.children_count; ++Idx)
		{
			CollectNode(Data, *Node.children[Idx], MirrorZ, OutScene);
		}
	}

	// cgltf_data is a C allocation with its own free function, which makes it the one thing
	// here that wants a guard rather than a destructor
	struct GltfDataGuard
	{
		DISABLE_COPY_MOVE(GltfDataGuard)

		cgltf_data* Data = nullptr;

		GltfDataGuard() = default;

		~GltfDataGuard()
		{
			if (Data != nullptr)
			{
				cgltf_free(Data);
			}
		}
	};
}

GltfScene LoadGltfFromFile(const std::wstring& Path)
{
	DXASSERT(!Path.empty(), "glTF loader needs a path");

	const std::string NarrowPath						= ToUtf8(Path);
	const std::filesystem::path BaseDir					= std::filesystem::path(Path).parent_path();

	cgltf_options Options = {};
	GltfDataGuard Guard;

	if (cgltf_parse_file(&Options, NarrowPath.c_str(), &Guard.Data) != cgltf_result_success)
	{
		throw std::runtime_error("Failed to parse glTF file: " + NarrowPath);
	}

	// The .gltf is only the description. Geometry lives in the buffers it points at, which
	// are a sidecar .bin here, and nothing can be read until those are in memory.
	if (cgltf_load_buffers(&Options, Guard.Data, NarrowPath.c_str()) != cgltf_result_success)
	{
		throw std::runtime_error("Failed to load glTF buffers for: " + NarrowPath);
	}

	if (cgltf_validate(Guard.Data) != cgltf_result_success)
	{
		throw std::runtime_error("glTF file failed validation: " + NarrowPath);
	}

	const cgltf_data& Data = *Guard.Data;
	GltfScene Scene;

	Scene.Materials.reserve(Data.materials_count);
	for (cgltf_size Idx = 0; Idx < Data.materials_count; ++Idx)
	{
		const cgltf_material& Source = Data.materials[Idx];

		GltfMaterial Material;
		Material.Name			= Source.name != nullptr ? FromUtf8(Source.name) : (L"Material_" + std::to_wstring(Idx));
		Material.NormalPath		= ResolveTexturePath(BaseDir, Source.normal_texture);
		Material.bDoubleSided	= Source.double_sided != 0;
		Material.bAlphaMask		= Source.alpha_mode == cgltf_alpha_mode_mask;

		if (Source.has_pbr_metallic_roughness)
		{
			Material.BaseColorPath			= ResolveTexturePath(BaseDir, Source.pbr_metallic_roughness.base_color_texture);
			Material.MetallicRoughnessPath	= ResolveTexturePath(BaseDir, Source.pbr_metallic_roughness.metallic_roughness_texture);
		}

		Scene.Materials.push_back(std::move(Material));
	}

	// Right handed with +Y up on the way in, left handed on the way out. Mirroring one axis
	// is what crosses between the two, and Z is the conventional one to pick because it
	// leaves up and right alone and so keeps the scene the way up the artist built it.
	const XMMATRIX MirrorZ = XMMatrixScaling(1.f, 1.f, -1.f);

	// Only the nodes a scene actually names are walked. A file may carry nodes no scene
	// refers to, and those are not part of it.
	if (Data.scene != nullptr)
	{
		for (cgltf_size Idx = 0; Idx < Data.scene->nodes_count; ++Idx)
		{
			CollectNode(Data, *Data.scene->nodes[Idx], MirrorZ, Scene);
		}
	}
	else
	{
		for (cgltf_size Idx = 0; Idx < Data.nodes_count; ++Idx)
		{
			if (Data.nodes[Idx].parent == nullptr)
			{
				CollectNode(Data, Data.nodes[Idx], MirrorZ, Scene);
			}
		}
	}

	if (Scene.Primitives.empty())
	{
		throw std::runtime_error("glTF file holds no triangles: " + NarrowPath);
	}

	size_t TotalVertices	= 0;
	size_t TotalIndices		= 0;
	XMVECTOR BoundsMin		= XMVectorReplicate(FLT_MAX);
	XMVECTOR BoundsMax		= XMVectorReplicate(-FLT_MAX);

	for (const GltfPrimitive& Primitive : Scene.Primitives)
	{
		TotalVertices	+= Primitive.Mesh.Vertices.size();
		TotalIndices	+= Primitive.Mesh.Indices.size();

		for (const TypeOfVertex& Vertex : Primitive.Mesh.Vertices)
		{
			const XMVECTOR Position = XMLoadFloat4(&Vertex.Position);
			BoundsMin = XMVectorMin(BoundsMin, Position);
			BoundsMax = XMVectorMax(BoundsMax, Position);
		}
	}

	XMFLOAT3 Min, Max;
	XMStoreFloat3(&Min, BoundsMin);
	XMStoreFloat3(&Max, BoundsMax);

	DXLOG("glTF: %u primitives, %u materials, %u vertices, %u triangles\n",
			static_cast<UINT>(Scene.Primitives.size()),
			static_cast<UINT>(Scene.Materials.size()),
			static_cast<UINT>(TotalVertices),
			static_cast<UINT>(TotalIndices / 3))
	DXLOG("glTF: bounds min (%.2f, %.2f, %.2f) max (%.2f, %.2f, %.2f)\n",
			Min.x, Min.y, Min.z, Max.x, Max.y, Max.z)

	return Scene;
}
