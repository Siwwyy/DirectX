#include "Primitive.h"

Primitive::Primitive()
{
    // set starting cubes position
    // first cube
    Matrices.Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);		// set object position
    XMVECTOR PosVec = XMLoadFloat4(&Matrices.Position);		// create xmvector for object position
    auto TmpMat = XMMatrixTranslationFromVector(PosVec);		// create translation matrix from object's position vector
    XMStoreFloat4x4(&Matrices.RotMat, XMMatrixIdentity());	// initialize object's rotation matrix to identity matrix
    XMStoreFloat4x4(&Matrices.WorldMat, TmpMat);				// store object's world matrix
}


HRESULT Primitive::Init(DXDevice* Device, DXGraphicsCommandList* CommandList)
{
    HRESULT hr = S_OK;
    return hr;
}