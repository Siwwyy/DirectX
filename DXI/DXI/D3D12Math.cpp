
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "D3D12Math.h"

ObjectMatrices::ObjectMatrices()
{
    Transform();
}

// ObjectMatrices Matrices
ObjectMatrices::ObjectMatrices(const DirectX::XMFLOAT3  InitRot,
                            const DirectX::XMFLOAT3     InitPosition,
                            const DirectX::XMFLOAT3     InitScale)
{
    Transform(InitRot, InitPosition, InitScale);
}

void ObjectMatrices::Transform(const XMFLOAT3 TranslateTransform,
    const XMFLOAT3 RotateTransform,
    const XMFLOAT3 ScaleTransform) noexcept
{
    // NOTE! Scale should be first always, then rotate and then translate

// NOTE! TODO! If rotatio if e.g., 720 degrees, there is no need
// to rotate object twice. Based on assumption, if we set rotation
// bigger than 360 degrees, do:
    const auto SubtractRotation = [](float Rotation) -> float
        {
            constexpr auto MaxRotation = 360.f;
            auto NewRotation = Rotation;
            if (Rotation > 360.f)
            {
                // 780 degrees of rotation / 360 ~= 2.16(6)
                // 2 * 360 = 720
                // 780 - 720 = 60
                // Final rotation should be 60 degrees only!
                UINT Multiplier = Rotation / MaxRotation;
                UINT MultipliedRotation = MaxRotation * Multiplier;
                NewRotation = Rotation - MultipliedRotation;
            }
            return NewRotation;
        };

    // set starting scale, rotation and position
    const auto ScaleVec = DirectX::XMVectorSet(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z, 1.f);
    const auto RotationVec = DirectX::XMVectorSet(SubtractRotation(RotateTransform.x), SubtractRotation(RotateTransform.y), SubtractRotation(RotateTransform.z), 1.f);
    const auto PositionOffsetVec = DirectX::XMVectorSet(TranslateTransform.x, TranslateTransform.y, TranslateTransform.z, 1.f);

    // create rotation for x,y,z
    const auto RotationXYZOld = XMLoadFloat4(&this->RotVec);
    const auto RotationXYZNew = RotationXYZOld + RotationVec;

    // Offset of translation
    const auto PositionOld = XMLoadFloat4(&this->PositionVec);
    const auto PositionNew = PositionOld + PositionOffsetVec;

    // Store
    XMStoreFloat4(&this->ScaleVec,      ScaleVec);
    XMStoreFloat4(&this->RotVec,        RotationXYZNew);
    XMStoreFloat4(&this->PositionVec,   PositionNew);

    // create xmmatrix for scale
    const auto ScaleMat = XMMatrixScaling(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z);

    // Create offset of rotation
    const auto RotationMatX = XMMatrixRotationX(this->RotVec.x);
    const auto RotationMatY = XMMatrixRotationY(this->RotVec.y);
    const auto RotationMatZ = XMMatrixRotationZ(this->RotVec.z);
    const auto RotationMatXYZ = RotationMatX * RotationMatY * RotationMatZ;

    // create translation matrix                                                                           
    const auto TranslationMat = XMMatrixTranslation(this->PositionVec.x, this->PositionVec.y, this->PositionVec.z);

    // Create World Matrix, Rotation * Position
    {
        const auto ScaleRotMat = ScaleMat * RotationMatXYZ;
        XMStoreFloat4x4(&this->ScaleRotMat, ScaleRotMat);
    }

    // Create World Matrix, Scale * Rotation * Position
    {
        const auto ScaleRotTranslationMat = ScaleMat * RotationMatXYZ * TranslationMat;
        XMStoreFloat4x4(&this->WorldMat, ScaleRotTranslationMat);
    }



#if 0


    // set starting scale, rotation and position
    const auto ScaleVec = DirectX::XMVectorSet(InitScale.x, InitScale.y, InitScale.z, 1.f);
    const auto RotationVec = DirectX::XMVectorSet(InitRot.x, InitRot.y, InitRot.z, 1.f);
    const auto PositionVec = DirectX::XMVectorSet(InitPosition.x, InitPosition.y, InitPosition.z, 1.f);

    // Store
    XMStoreFloat4(&this->ScaleVec, ScaleVec);
    XMStoreFloat4(&this->RotVec, RotationVec);
    XMStoreFloat4(&this->PositionVec, PositionVec);

    // create xmvector for object world matrix
    const auto ScaleMat = XMMatrixScaling(InitScale.x, InitScale.y, InitScale.z);
    const auto RotationMatX = XMMatrixRotationX(Helpers::ToRadians(InitRot.x));
    const auto RotationMatY = XMMatrixRotationY(Helpers::ToRadians(InitRot.y));
    const auto RotationMatZ = XMMatrixRotationZ(Helpers::ToRadians(InitRot.z));
    const auto RotationMatXYZ = RotationMatX * RotationMatY * RotationMatZ;

    //const auto WorldMatrix      = (ScaleMat * RotationMatXYZ) * PositionMat; // TODO | Rethink if it should not go in reverse order like f(g(z(x))) <==> Pos(Rot(Scale(x)))

    // create translation matrix                                                                           
    const auto TranslationMat = XMMatrixTranslation(InitPosition.x, InitPosition.y, InitPosition.z);

    // Create World Matrix, Rotation * Position
    {
        const auto ScaleRotMat = ScaleMat * RotationMatXYZ;
        XMStoreFloat4x4(&this->ScaleRotMat, ScaleRotMat);
    }

    // Create World Matrix, Scale * Rotation * Position
    {
        const auto ScaleRotTranslationMat = ScaleMat * RotationMatXYZ * TranslationMat;
        XMStoreFloat4x4(&this->WorldMat, ScaleRotTranslationMat);
    }
#endif
}