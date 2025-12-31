#include "Primitive.h"

// Ctors
Primitive::Primitive()
    : VertexBufferUpload({})
    , VertexBuffer({})
    , VertexBufferView({})
    , IndexBufferUpload({})
    , IndexBuffer({})
    , IndexBufferView({})
    , NumIndices(0)
    , Matrices()
{ }

Primitive::Primitive(const XMFLOAT4 InitRot,
    const XMFLOAT4 InitPosition,
    const XMFLOAT4 InitScale)
    : VertexBufferUpload({})
    , VertexBuffer({})
    , VertexBufferView({})
    , IndexBufferUpload({})
    , IndexBuffer({})
    , IndexBufferView({})
    , NumIndices(0)
    , Matrices(InitRot, InitPosition, InitScale)
{ }

// Functions
HRESULT Primitive::Init(DXDevice* Device, DXGraphicsCommandList* CommandList)
{
    HRESULT hr = S_OK;
    return hr;
}

void Primitive::Transform(const XMFLOAT3 TranslateTransform,
    const XMFLOAT3 RotateTransform,
    const XMFLOAT3 ScaleTransform)
{
    // NOTE! Scale should be first alwayas, then rotate and then translate

    // Scaling
    const auto ScalingMatrix = XMMatrixScaling(ScaleTransform.x,
                                                ScaleTransform.y,
                                                ScaleTransform.z);
    XMStoreFloat4x4(&Matrices.ScaleMat, ScalingMatrix);

    // Rotating
    const auto RotationXMatrix = XMMatrixRotationX(RotateTransform.x);
    const auto RotationYMatrix = XMMatrixRotationY(RotateTransform.y);
    const auto RotationZMatrix = XMMatrixRotationZ(RotateTransform.z);


    // Translating
    const auto TranslationOffset    = XMMatrixTranslationFromVector(DirectX::XMVectorSet(TranslateTransform.x, TranslateTransform.y, TranslateTransform.z, 1.f));
    const auto TranslateMatrix      = XMLoadFloat4x4(&Matrices.PositionMat) * TranslationOffset;
    XMStoreFloat4x4(&Matrices.PositionMat, TranslateMatrix);

    // Combine matrices together
    const auto RotationMatrixCombined = XMLoadFloat4x4(&Matrices.RotMat) * RotationXMatrix * RotationYMatrix * RotationZMatrix;
    XMStoreFloat4x4(&Matrices.RotMat, RotationMatrixCombined);

    //Create World Mat
    const auto WorldMat = ScalingMatrix * RotationMatrixCombined * TranslateMatrix;
    XMStoreFloat4x4(&Matrices.WorldMat, WorldMat);
}