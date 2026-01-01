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

    // set starting scale, rotation and position
    const auto ScaleVec             = DirectX::XMVectorSet(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z, 1.f);
    const auto RotationVec          = DirectX::XMVectorSet(RotateTransform.x, RotateTransform.y, RotateTransform.z, 1.f);
    const auto PositionOffsetVec    = DirectX::XMVectorSet(TranslateTransform.x, TranslateTransform.y, TranslateTransform.z, 1.f);

    // create xmvector for object world matrix
    const auto ScaleMat             = XMMatrixScaling(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z);
    const auto RotationMatX         = XMMatrixRotationX(RotateTransform.x);
    const auto RotationMatY         = XMMatrixRotationY(RotateTransform.y);
    const auto RotationMatZ         = XMMatrixRotationZ(RotateTransform.z);
    const auto RotationMatXYZ       = RotationMatX * RotationMatY * RotationMatZ;

    // Offset of translation
    const auto PositionOld          = XMLoadFloat4(&Matrices.PositionVec);
    const auto PositionNew          = PositionOld + PositionOffsetVec;
    
    // Store
    XMStoreFloat4(&Matrices.ScaleVec,       ScaleVec);
    XMStoreFloat4(&Matrices.RotVec,         RotationVec);
    XMStoreFloat4(&Matrices.PositionVec,    PositionNew);

    // create translation matrix                                                                               // initialize object's rotation matrix to identity matrix
    const auto PositionMat = XMMatrixTranslation(Matrices.PositionVec.x, Matrices.PositionVec.y, Matrices.PositionVec.z);
    
    // Create World Matrix, Scale * Rotation * Position
    const auto WorldMatrix = ScaleMat * RotationMatXYZ * PositionMat;
    XMStoreFloat4x4(&Matrices.WorldMat, WorldMatrix);
}