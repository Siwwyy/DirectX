#include "Primitive.h"

// Ctors
Primitive::Primitive()
    : VertexFactory({})
    , IndexBufferUpload({})
    , IndexBuffer({})
    , IndexBufferView({})
    , NumIndices(0)
    , Matrices()
{ }

Primitive::Primitive(std::wstring Name):
    VertexFactory({})
    , IndexBufferUpload({})
    , IndexBuffer({})
    , IndexBufferView({})
    , NumIndices(0)
    , Matrices()
	, Name(std::move(Name))
{ }

Primitive::Primitive(const XMFLOAT3 InitRot,
    const XMFLOAT3 InitPosition,
    const XMFLOAT3 InitScale)
    : VertexFactory({})
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
    // Transform
    Matrices.Transform(TranslateTransform, RotateTransform, ScaleTransform);

#if 0
    // NOTE! Scale should be first always, then rotate and then translate

    // NOTE! TODO! If rotatio if e.g., 720 degrees, there is no need
    // to rotate object twice. Based on assumption, if we set rotation
    // bigger than 360 degrees, do:
    const auto SubtractRotation = [](float Rotation) -> float
    {
        constexpr auto MaxRotation  = 360.f;
        auto NewRotation            = Rotation;
        if (Rotation > 360.f)
        {
            // 780 degrees of rotation / 360 ~= 2.16(6)
            // 2 * 360 = 720
            // 780 - 720 = 60
            // Final rotation should be 60 degrees only!
            UINT Multiplier         = Rotation / MaxRotation;
            UINT MultipliedRotation = MaxRotation * Multiplier;
            NewRotation             = Rotation - MultipliedRotation;
        }
        return NewRotation;
    };

    // set starting scale, rotation and position
    const auto ScaleVec             = DirectX::XMVectorSet(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z, 1.f);
    const auto RotationVec          = DirectX::XMVectorSet(SubtractRotation(RotateTransform.x), SubtractRotation(RotateTransform.y), SubtractRotation(RotateTransform.z), 1.f);
    const auto PositionOffsetVec    = DirectX::XMVectorSet(TranslateTransform.x, TranslateTransform.y, TranslateTransform.z, 1.f);    
  
    // create xmmatrix for scale
    const auto ScaleMat             = XMMatrixScaling(ScaleTransform.x, ScaleTransform.y, ScaleTransform.z);
    
    // create rotation for x,y,z
    const auto RotationXYZOld       = XMLoadFloat4(&Matrices.RotVec);
    const auto RotationXYZNew       = RotationXYZOld + RotationVec;

    // Offset of translation
    const auto PositionOld          = XMLoadFloat4(&Matrices.PositionVec);
    const auto PositionNew          = PositionOld + PositionOffsetVec;
    
    Matrices.


    // Store
    XMStoreFloat4(&Matrices.ScaleVec,       ScaleVec);
    XMStoreFloat4(&Matrices.RotVec,         RotationXYZNew);
    XMStoreFloat4(&Matrices.PositionVec,    PositionNew);

    // Create offset of rotation
    const auto RotationMatX     = XMMatrixRotationX(Matrices.RotVec.x);
    const auto RotationMatY     = XMMatrixRotationY(Matrices.RotVec.y);
    const auto RotationMatZ     = XMMatrixRotationZ(Matrices.RotVec.z);
    const auto RotationMatXYZ   = RotationMatX * RotationMatY * RotationMatZ;

    // create translation matrix                                                                           
    const auto TranslationMat = XMMatrixTranslation(Matrices.PositionVec.x, Matrices.PositionVec.y, Matrices.PositionVec.z);
    
    // Create World Matrix, Rotation * Position
    {
        const auto ScaleRotMat = ScaleMat * RotationMatXYZ;
        XMStoreFloat4x4(&Matrices.ScaleRotMat, ScaleRotMat);
    }

    // Create World Matrix, Scale * Rotation * Position
    {
        const auto ScaleRotTranslationMat = ScaleMat * RotationMatXYZ * TranslationMat;
        XMStoreFloat4x4(&Matrices.WorldMat, ScaleRotTranslationMat);
    }

#endif
}