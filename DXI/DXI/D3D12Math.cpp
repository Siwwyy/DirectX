
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Math.h"

ObjectMatrices::ObjectMatrices()
{
    Init();
}

// ObjectMatrices Matrices
ObjectMatrices::ObjectMatrices(const DirectX::XMFLOAT4 InitRot,
    const DirectX::XMFLOAT4 InitPosition, 
    const DirectX::XMFLOAT4 InitScale) 
{
    Init(InitRot, InitPosition, InitScale);
}

void ObjectMatrices::Init(const DirectX::XMFLOAT4 InitRot, 
    const DirectX::XMFLOAT4 InitPosition, 
    const DirectX::XMFLOAT4 InitScale) noexcept
{
    // set starting scale, rotation and position
    const auto ScaleVec     = DirectX::XMVectorSet(InitScale.x, InitScale.y, InitScale.z, 1.f);
    const auto RotationVec  = DirectX::XMVectorSet(InitRot.x, InitRot.y, InitRot.z, 1.f);
    const auto PositionVec  = DirectX::XMVectorSet(InitPosition.x, InitPosition.y, InitPosition.z, 1.f);

    // Store
    XMStoreFloat4x4(&ScaleMat,      XMMatrixScalingFromVector(ScaleVec));
    XMStoreFloat4x4(&RotMat,        XMMatrixRotationRollPitchYawFromVector(RotationVec));	                                                                                // initialize object's rotation matrix to identity matrix
    XMStoreFloat4x4(&PositionMat,   XMMatrixTranslationFromVector(PositionVec));// set object position

    // create xmvector for object world matrix
    auto WorldMatrix = XMLoadFloat4x4(&ScaleMat) *
                       XMLoadFloat4x4(&RotMat) *
                       XMLoadFloat4x4(&PositionMat);
    // create translation matrix from object's position vector, rotation is identity                                                                                // initialize object's rotation matrix to identity matrix
    XMStoreFloat4x4(&WorldMat, WorldMatrix);
}
