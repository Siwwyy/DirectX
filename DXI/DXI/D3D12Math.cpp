
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Math.h"

// ObjectMatrices Matrices
ObjectMatrices::ObjectMatrices(const DirectX::XMFLOAT4X4 InitWorldMat, const DirectX::XMFLOAT4X4 InitRotMat, const DirectX::XMFLOAT4 InitPosition, const DirectX::XMFLOAT4 InitScale) 
	: WorldMat(InitWorldMat)
	, RotMat(InitRotMat)
	, Position(InitPosition)
	, Scale(InitScale)
{}
