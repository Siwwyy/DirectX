//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_PRIMITIVE_H_INCLUDED
#define D3D12_PRIMITIVE_H_INCLUDED

#include "../pch.h"
#include "../D3D12Helpers.h"
#include "../D3D12Math.h"

class Primitive
{
	DISABLE_COPY(Primitive)

public:

	Primitive();
	Primitive(const DirectX::XMFLOAT4 InitRot,
			  const DirectX::XMFLOAT4 InitPosition,
			  const XMFLOAT4 InitScale);
	~Primitive() = default;

	// Init
	virtual HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList);

	// Setters
	void SetNumIndices(const UINT NumIndices)
	{
		this->NumIndices = NumIndices;
	}

	// Main Transform of pritmitive
	void Transform(const XMFLOAT3 TranslateTransform	= DX_IDENTITY_TRANSFORM3,
				   const XMFLOAT3 RotateTransform		= DX_IDENTITY_ROTATE3,
				   const XMFLOAT3 ScaleTransform		= DX_IDENTITY_SCALE3);
	
	// Getters
	_NODISCARD inline const auto GetWorldMatrix() const
	{
		return Matrices.WorldMat;
	}
	_NODISCARD inline const auto GetPosVector() const
	{
		return Matrices.PositionMat;
	}
	_NODISCARD inline const auto GetRotationMatrix() const
	{
		return Matrices.RotMat;
	}
	_NODISCARD inline const auto GetNumIndices() const
	{
		return NumIndices;
	}

	// D3D12 Vertex data
	ComPtr<DXResource>			VertexBufferUpload;
	ComPtr<DXResource>			VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView;

	// D3D12 Index buffer data
	ComPtr<DXResource>			IndexBufferUpload;
	ComPtr<DXResource>			IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW		IndexBufferView;

private:

	/* [[deprecated("SHOULD NOT BE USED")]] */ void SetWorldMatrix(const XMMATRIX Matrix)
	{
		XMStoreFloat4x4(&Matrices.WorldMat, Matrix);
	}

	/* [[deprecated("SHOULD NOT BE USED")]] */ void SetPosVector(const XMFLOAT4 Vector)
	{
		const auto PositionVec = DirectX::XMVectorSet(Vector.x, Vector.y, Vector.z, 1.f);
		XMStoreFloat4x4(&Matrices.PositionMat, XMMatrixTranslationFromVector(PositionVec));
	}

	/* [[deprecated("SHOULD NOT BE USED")]] */ void SetRotationMatrix(const XMMATRIX Matrix)
	{
		XMStoreFloat4x4(&Matrices.RotMat, Matrix);
	}


	// Matrices
	ObjectMatrices				Matrices;

	// Number of indices
	UINT NumIndices;

};


#endif // D3D12_PRIMITIVE_H_INCLUDED