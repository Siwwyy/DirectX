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
	~Primitive()	= default;

	virtual HRESULT Init(DXDevice* Device, DXGraphicsCommandList* CommandList);

	void SetWorldMatrix(const XMMATRIX Matrix)
	{
		XMStoreFloat4x4(&Matrices.WorldMat, Matrix);
	}

	void SetPosVector(const XMFLOAT4 Vector)
	{
		Matrices.Position = Vector;
	}

	void SetRotationMatrix(const XMMATRIX Matrix)
	{
		XMStoreFloat4x4(&Matrices.RotMat, Matrix);
	}

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


};


#endif // D3D12_PRIMITIVE_H_INCLUDED