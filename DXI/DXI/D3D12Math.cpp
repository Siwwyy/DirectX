
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

    // create xmvector for object world matrix
    const auto ScaleMat         = XMMatrixScaling(InitScale.x, InitScale.y, InitScale.z);
    const auto RotationMatX     = XMMatrixRotationX(Helpers::ToRadians(InitRot.x));
    const auto RotationMatY     = XMMatrixRotationY(Helpers::ToRadians(InitRot.y));
    const auto RotationMatZ     = XMMatrixRotationZ(Helpers::ToRadians(InitRot.z));
    const auto RotationMatXYZ   = RotationMatX * RotationMatY * RotationMatZ;
    const auto PositionMat      = XMMatrixTranslation(InitPosition.x, InitPosition.y, InitPosition.z);
    const auto WorldMatrix      = ScaleMat * RotationMatXYZ * PositionMat; // TODO | Rethink if it should not go in reverse order like f(g(z(x))) <==> Pos(Rot(Scale(x)))
    // Store
    XMStoreFloat4(&this->ScaleVec,      ScaleVec);
    XMStoreFloat4(&this->RotVec,        RotationVec);	                                                                                // initialize object's rotation matrix to identity matrix
    XMStoreFloat4(&this->PositionVec,   PositionVec);
    // create translation matrix from object's position vector, rotation is identity                                                                                // initialize object's rotation matrix to identity matrix
    XMStoreFloat4x4(&WorldMat, WorldMatrix);
}
