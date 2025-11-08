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
	Primitive(const DirectX::XMFLOAT4X4 InitWorldMat,
				const DirectX::XMFLOAT4X4 InitRotMat,
				const DirectX::XMFLOAT4 InitPosition,
				const XMFLOAT4 InitScale);
	~Primitive() = default;

	virtual HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList);

	/* [[deprecated("SHOULD NOT BE USED")]] */ void SetWorldMatrix(const XMMATRIX Matrix)
	{
		XMStoreFloat4x4(&Matrices.WorldMat, Matrix);
	}

	/* [[deprecated("SHOULD NOT BE USED")]] */ void SetPosVector(const XMFLOAT4 Vector)
	{
		Matrices.Position = Vector;
	}

	/* [[deprecated("SHOULD NOT BE USED")]] */ void SetRotationMatrix(const XMMATRIX Matrix)
	{
		XMStoreFloat4x4(&Matrices.RotMat, Matrix);
	}

	void SetNumIndices(const UINT NumIndices)
	{
		this->NumIndices = NumIndices;
	}

	void Transform(const XMFLOAT3 ScaleTransform		= XMFLOAT3(1.f, 1.f, 1.f),
					const XMFLOAT3 RotateTransform		= XMFLOAT3(0.f, 0.f, 0.f),
					const XMFLOAT3 TranslateTransform	= XMFLOAT3(0.f, 0.f, 0.f));

	_NODISCARD inline const auto GetWorldMatrix() const
	{
		return Matrices.WorldMat;
	}
	_NODISCARD inline const auto GetPosVector() const
	{
		return Matrices.Position;
	}
	_NODISCARD inline const auto GetRotationMatrix() const
	{
		return Matrices.RotMat;
	}
	_NODISCARD inline const auto GetNumIndices() const
	{
		return NumIndices;
	}

	void PrintTransformPretty() const;

	// D3D12 Vertex data
	ComPtr<DXResource>			VertexBufferUpload;
	ComPtr<DXResource>			VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView;

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