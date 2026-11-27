
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#include "DXIMath.h"

#include <algorithm>

ObjectMatrices::ObjectMatrices()
    : RotVec(DX_IDENTITY_ROTATE4)
    , ScaleVec(DX_IDENTITY_SCALE4)
    , PositionVec(DX_IDENTITY_TRANSLATE4)
{
    SetTransform();
}

// ObjectMatrices Matrices
ObjectMatrices::ObjectMatrices(const DirectX::XMFLOAT3  InitRot,
                            const DirectX::XMFLOAT3     InitPosition,
                            const DirectX::XMFLOAT3     InitScale)
    : ObjectMatrices()
{
    SetTransform(InitRot, InitPosition, InitScale);
}

void ObjectMatrices::SetTransform(const XMFLOAT3 TranslateTransform,
    const XMFLOAT3 RotateTransform,
    const XMFLOAT3 ScaleTransform) noexcept
{
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

    // What is there now plus the step. Scale is not added, it replaces.
    const XMFLOAT3 PositionNew( this->PositionVec.x + TranslateTransform.x,
                                this->PositionVec.y + TranslateTransform.y,
                                this->PositionVec.z + TranslateTransform.z);
    const XMFLOAT3 RotationNew( this->RotVec.x + SubtractRotation(RotateTransform.x),
                                this->RotVec.y + SubtractRotation(RotateTransform.y),
                                this->RotVec.z + SubtractRotation(RotateTransform.z));

    SetAbsoluteTransform(PositionNew, RotationNew, ScaleTransform);
}

void ObjectMatrices::SetAbsoluteTransform(const XMFLOAT3 Position,
    const XMFLOAT3 RotationDegrees,
    const XMFLOAT3 Scale) noexcept
{
    // Store
    this->ScaleVec      = XMFLOAT4(Scale.x, Scale.y, Scale.z, 1.f);
    this->RotVec        = XMFLOAT4(RotationDegrees.x, RotationDegrees.y, RotationDegrees.z, 1.f);
    this->PositionVec   = XMFLOAT4(Position.x, Position.y, Position.z, 1.f);

    // NOTE! Scale should be first always, then rotate and then translate
    const auto ScaleMat         = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
    const auto RotationMatXYZ   = RotationFromEulerDegrees(RotationDegrees);
    const auto TranslationMat   = XMMatrixTranslation(Position.x, Position.y, Position.z);

    // Create Local Matrix, Scale * Rotation
    XMStoreFloat4x4(&this->ScaleRotMat, ScaleMat * RotationMatXYZ);

    // Create World Matrix, Scale * Rotation * Position
    XMStoreFloat4x4(&this->WorldMat, ScaleMat * RotationMatXYZ * TranslationMat);
}

XMMATRIX RotationFromEulerDegrees(const XMFLOAT3& Degrees)
{
    const auto RotationMatX = XMMatrixRotationX(Helpers::ToRadians(Degrees.x));
    const auto RotationMatY = XMMatrixRotationY(Helpers::ToRadians(Degrees.y));
    const auto RotationMatZ = XMMatrixRotationZ(Helpers::ToRadians(Degrees.z));
    return RotationMatX * RotationMatY * RotationMatZ;
}

XMMATRIX NormalMatrixFromWorld(FXMMATRIX World)
{
    // Only the 3x3 matters for a direction. Dropping the translation first also keeps the inverse
    // free of it, so no stray values land in the w column once it is transposed.
    XMMATRIX Linear = World;
    Linear.r[0] = XMVectorSetW(Linear.r[0], 0.f);
    Linear.r[1] = XMVectorSetW(Linear.r[1], 0.f);
    Linear.r[2] = XMVectorSetW(Linear.r[2], 0.f);
    Linear.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    XMVECTOR Determinant;
    const XMMATRIX Inverse = XMMatrixInverse(&Determinant, Linear);

    // A singular matrix has no inverse. XMMatrixInverse hands back NaN with a determinant of 0
    if (XMVector3NearEqual(Determinant, XMVectorZero(), XMVectorReplicate(1e-12f)))
    {
        return Linear;
    }

    return XMMatrixTranspose(Inverse);
}

XMFLOAT3 EulerDegreesFromRotation(FXMMATRIX Rotation)
{
    // The product Rx(a) * Ry(b) * Rz(c) written out, row vector convention, c and s for cosine and sine:
    //
    //      row 0 | cb*cc                 cb*sc                 -sb
    //      row 1 | sa*sb*cc - ca*sc      sa*sb*sc + ca*cc      sa*cb
    //      row 2 | ca*sb*cc + sa*sc      ca*sb*sc - sa*cc      ca*cb
    //
    // so -sb is read straight off [0][2], and a and c off the entries that carry cb alone.
    XMFLOAT4X4 M;
    XMStoreFloat4x4(&M, Rotation);

    // cb is the length of the first two entries of row 0, so b comes from an atan2 of the two. Not
    // from asin of -sb: asin is steep near +-1 and throws away half the digits there, which is
    // exactly where the rotation is hardest to read.
    const float CosB = sqrtf((M._11 * M._11) + (M._12 * M._12));
    const float B    = atan2f(-M._13, CosB);

    float A = 0.f;
    float C = 0.f;

    if (CosB > 1e-4f)
    {
        A = atan2f(M._23, M._33);
        C = atan2f(M._12, M._11);
    }
    else
    {
        // Looking along the Y axis: cb is zero, X and Z spin the same axis and only a plus or
        // minus c is left to read. Put it all on X, which with c at zero leaves
        // row 1 as [sa*sb, ca, ...] to read it from, and sb is plus or minus one.
        const float SinB = M._13 < 0.f ? 1.f : -1.f;
        A = atan2f(SinB * M._21, M._22);
        C = 0.f;
    }

    return XMFLOAT3(XMConvertToDegrees(A), XMConvertToDegrees(B), XMConvertToDegrees(C));
}
