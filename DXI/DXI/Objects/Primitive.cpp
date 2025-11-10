#include "Primitive.h"

Primitive::Primitive()
{
    // set starting cubes position
    // first cube
    Matrices.Scale      = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);		                                                                            // set object scale
    Matrices.Position   = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);		                                                                            // set object position
    XMVECTOR PosVec     = XMLoadFloat4(&Matrices.Position);		                                                                            // create xmvector for object position
    auto TmpMat         = XMMatrixScaling(Matrices.Scale.x, Matrices.Scale.y, Matrices.Scale.z) * XMMatrixTranslationFromVector(PosVec);	// create translation matrix from object's position vector
    XMStoreFloat4x4(&Matrices.RotMat, XMMatrixIdentity());	                                                                                // initialize object's rotation matrix to identity matrix
    XMStoreFloat4x4(&Matrices.WorldMat, TmpMat);				                                                                            // store object's world matrix
}

Primitive::Primitive(const XMFLOAT4X4 InitWorldMat,
    const XMFLOAT4X4 InitRotMat,
    const XMFLOAT4 InitPosition,
    const XMFLOAT4 InitScale)
    : Matrices(InitWorldMat, InitRotMat, InitPosition, InitScale)
{

}


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

    // Rotating
    const auto RotationXMatrix = XMMatrixRotationX(RotateTransform.x);
    const auto RotationYMatrix = XMMatrixRotationY(RotateTransform.y);
    const auto RotationZMatrix = XMMatrixRotationZ(RotateTransform.z);


    // Translating
    Matrices.Position.x += TranslateTransform.x;
    Matrices.Position.y += TranslateTransform.y;
    Matrices.Position.z += TranslateTransform.z;
    const auto TranslateMatrix = XMMatrixTranslation(Matrices.Position.x,
                                                    Matrices.Position.y,
                                                    Matrices.Position.z);

    // Combine matrices together
    const auto RotationMatrixCombined = XMLoadFloat4x4(&Matrices.RotMat) * RotationXMatrix * RotationYMatrix * RotationZMatrix;
    SetRotationMatrix(RotationMatrixCombined);

    const auto WorldMat = ScalingMatrix * RotationMatrixCombined * TranslateMatrix;
    SetWorldMatrix(WorldMat);
}

void Primitive::PrintTransformPretty() const
{
    DXLOG("Transform %f %f %f \n", Matrices.Position.x, Matrices.Position.y, Matrices.Position.z)
}
