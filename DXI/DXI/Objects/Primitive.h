//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_PRIMITIVE_H_INCLUDED
#define D3D12_PRIMITIVE_H_INCLUDED

#include "pch.h"
#include "DXIHelpers.h"
#include "DXIMath.h"
#include "Vertex.h"

// FWD
class Material;

// Primitive class
class Primitive
{
	DISABLE_COPY(Primitive)

public:

	Primitive();
	Primitive(std::wstring Name);
	Primitive(const XMFLOAT3 InitRot,
			const XMFLOAT3 InitPosition,
			const XMFLOAT3 InitScale);
	virtual ~Primitive() = default;

	/**
	 * Init
	 */
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

	// Main Transform of pritmitive | Nudges position and rotation, replaces scale
	void SetTransform(const XMFLOAT3 TranslateTransform	= DX_IDENTITY_TRANSLATE3,
				      const XMFLOAT3 RotateTransform	= DX_IDENTITY_ROTATE3,
				      const XMFLOAT3 ScaleTransform		= DX_IDENTITY_SCALE3);

	// Where it should end up, nothing is added to what is there. Rotation in degrees.
	// For a widget or a gizmo that holds the value itself, where SetTransform would stack
	// its answer on top of the old one.
	void SetAbsoluteTransform(const XMFLOAT3 Position,
							  const XMFLOAT3 RotationDegrees,
							  const XMFLOAT3 Scale)
	{
		Matrices.SetAbsoluteTransform(Position, RotationDegrees, Scale);
	}

	// Getters
	_NODISCARD inline const auto GetWorldMatrixNoTranslation() const
	{
		return Matrices.ScaleRotMat;
	}
	_NODISCARD inline const auto GetWorldMatrix() const
	{
		return Matrices.WorldMat;
	}
	_NODISCARD inline const auto GetPrevWorldMatrix() const
	{
		return Matrices.PrevWorldMat;
	}

	/**
	 * Rolls this frame's transform forward into next frame's previous one.
	 *
	 * Once per frame, after the constants have been uploaded and before anything moves the
	 * object. Also once when the object is first placed, otherwise it reports a frame of
	 * motion from wherever it was created to where it was put.
	 */
	void AdvanceFrame() noexcept
	{
		Matrices.PrevWorldMat = Matrices.WorldMat;
	}
	_NODISCARD inline const auto GetPosVector() const
	{
		return Matrices.PositionVec;
	}
	_NODISCARD inline const auto GetRotVector() const
	{
		return Matrices.RotVec;
	}
	_NODISCARD inline const auto GetScaleVector() const
	{
		return Matrices.ScaleVec;
	}
	_NODISCARD inline const auto GetNumIndices() const
	{
		return NumIndices;
	}
	_NODISCARD inline auto GetMaterial() const
	{
		// Falls back to the default, so a freshly created mesh is always drawable
		return PrimitiveMaterial ? PrimitiveMaterial : DefaultMaterial; 
	}
	_NODISCARD inline bool HasOwnMaterial() const
	{
		return PrimitiveMaterial != nullptr;
	}

	/**
	 * Whether the tangent frame overlay draws the vectors of every vertex, whatever share the
	 * panel asks for. For the built in shapes, which are few vertices that are worth seeing
	 * all of: a cube has 24, four to a face, since each face needs its own normals. A mesh with
	 * more than that is a sample by default, a mesh of up to TANGENT_FRAME_SHOW_ALL_VERTEX_LIMIT
	 * vertices shows all of them on its own.
	 */
	_NODISCARD virtual bool AlwaysShowsAllFrameVertices() const noexcept
	{
		return false;
	}

	// D3D12 Vertex data
	VertexFactory				VertexFactory;

	// D3D12 Index buffer data
	ComPtr<DXResource>			IndexBufferUpload;
	ComPtr<DXResource>			IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW		IndexBufferView;

	// Name of primitive (for debug purposes)
	std::wstring				Name = L"Primitive";

	// Whether the tangent frame overlay draws this object's vectors. Per object so any set of
	// them can be inspected at once, the overlay's own settings only say how they look.
	bool						bShowTangentFrame = false;

	// Same idea for the three arrows at the object's origin, a flag per object and the
	// overlay's own settings for how they look
	bool						bShowOriginAxes = false;

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
