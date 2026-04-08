//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_PRIMITIVE_H_INCLUDED
#define D3D12_PRIMITIVE_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "../D3D12Math.h"
#include "../Geometry/Vertex.h"

class Primitive
{
	DISABLE_COPY(Primitive)

public:

	Primitive();
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

	// Main Transform of pritmitive
	void Transform(const XMFLOAT3 TranslateTransform	= DX_IDENTITY_TRANSLATE3,
				   const XMFLOAT3 RotateTransform		= DX_IDENTITY_ROTATE3,
				   const XMFLOAT3 ScaleTransform		= DX_IDENTITY_SCALE3);
	
	// Getters
	_NODISCARD inline const auto GetScaleRotMatrix() const
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

	// D3D12 Vertex data
	VertexFactory				VertexFactory;

	// D3D12 Index buffer data
	ComPtr<DXResource>			IndexBufferUpload;
	ComPtr<DXResource>			IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW		IndexBufferView;

private:

	// Matrices
	ObjectMatrices				Matrices;

	// Number of indices
	UINT NumIndices;
};

#endif // D3D12_PRIMITIVE_H_INCLUDED