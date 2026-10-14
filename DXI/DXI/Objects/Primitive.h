//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_PRIMITIVE_H_INCLUDED
#define D3D12_PRIMITIVE_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "../D3D12Math.h"
#include "../Geometry/Vertex.h"

class Material;

class Primitive
{
	DISABLE_COPY(Primitive)

public:

	Primitive();
	Primitive(std::wstring Name);
	Primitive(const XMFLOAT3 InitRot,
			const XMFLOAT3 InitPosition,
			const XMFLOAT3 InitScale);
	~Primitive() = default;

	// Init
	virtual HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList);

	// Setters
	void SetNumIndices(const UINT NumIndices)
	{
		this->NumIndices = NumIndices;
	}

	// Material this primitive is drawn with, owned by whoever created it.
	// Overrides the default, pass nullptr to fall back to it again.
	void SetMaterial(Material * const InMaterial)
	{
		this->PrimitiveMaterial = InMaterial;
	}

	// Material every primitive without one of its own is drawn with. Registered once by
	// whoever owns the materials, before anything is drawn.
	static void SetDefaultMaterial(Material * const InDefaultMaterial)
	{
		DefaultMaterial = InDefaultMaterial;
	}

	// Main Transform of pritmitive
	void Transform(const XMFLOAT3 TranslateTransform	= DX_IDENTITY_TRANSLATE3,
				   const XMFLOAT3 RotateTransform		= DX_IDENTITY_ROTATE3,
				   const XMFLOAT3 ScaleTransform		= DX_IDENTITY_SCALE3);
	
	// Getters
	_NODISCARD inline const auto GetWorldMatrixNoTranslation() const
	{
		return Matrices.ScaleRotMat;
	}
	_NODISCARD inline const auto GetWorldMatrix() const
	{
		return Matrices.WorldMat;
	}
	_NODISCARD inline const auto GetPosVector() const
	{
		return Matrices.PositionVec;
	}
	_NODISCARD inline const auto GetNumIndices() const
	{
		return NumIndices;
	}
	// Falls back to the default, so a freshly created mesh is always drawable
	_NODISCARD inline auto GetMaterial() const
	{
		return PrimitiveMaterial ? PrimitiveMaterial : DefaultMaterial;
	}
	_NODISCARD inline bool HasOwnMaterial() const
	{
		return PrimitiveMaterial != nullptr;
	}

	// D3D12 Vertex data
	VertexFactory				VertexFactory;

	// D3D12 Index buffer data
	ComPtr<DXResource>			IndexBufferUpload;
	ComPtr<DXResource>			IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW		IndexBufferView;

	// Name of primitive (for debug purposes)
	std::wstring				Name = L"Primitive";

private:

	// Matrices
	ObjectMatrices				Matrices;

	// Number of indices
	UINT						NumIndices;

	// Material of the primitive, not owned here. Null means use the default below.
	Material *					PrimitiveMaterial = nullptr;

	// Shared by every primitive, also not owned here
	static Material *			DefaultMaterial;
};

#endif // D3D12_PRIMITIVE_H_INCLUDED