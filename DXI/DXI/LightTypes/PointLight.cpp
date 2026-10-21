//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#include "PointLight.h"

PointLight::PointLight()
	: LightBase(L"Point Light")
{
}

PointLight::PointLight(std::wstring Name)
	: LightBase(std::move(Name))
{
}

void PointLight::SetRange(const float Range) noexcept
{
	DXASSERT(Range > 0.f, "Point light range must be greater than zero, the shader divides by it");
	this->Range = Range;
}

void PointLight::SetSourceRadius(const float SourceRadius) noexcept
{
	DXASSERT(SourceRadius > 0.f, "Source radius must be greater than zero, it is what keeps the falloff finite");
	this->SourceRadius = SourceRadius;
}

XMMATRIX PointLight::GetShadowViewProj(const UINT FaceIdx) const noexcept
{
	DXASSERT(FaceIdx < 6, "A cube has six faces, the index has to be one of them");

	const auto Eye = XMLoadFloat3(&Position);

	// Where each cube face looks, and which way is up for it. Same order the cube
	// sampler expects, so a direction lookup lands on the face that rendered it.
	static const XMVECTORF32 FaceDirections[6] =
	{
		{  1.f,  0.f,  0.f, 0.f },	// +X
		{ -1.f,  0.f,  0.f, 0.f },	// -X
		{  0.f,  1.f,  0.f, 0.f },	// +Y
		{  0.f, -1.f,  0.f, 0.f },	// -Y
		{  0.f,  0.f,  1.f, 0.f },	// +Z
		{  0.f,  0.f, -1.f, 0.f }	// -Z
	};

	static const XMVECTORF32 FaceUps[6] =
	{
		{  0.f,  1.f,  0.f, 0.f },
		{  0.f,  1.f,  0.f, 0.f },
		{  0.f,  0.f, -1.f, 0.f },	// looking straight up, so up has to point somewhere else
		{  0.f,  0.f,  1.f, 0.f },
		{  0.f,  1.f,  0.f, 0.f },
		{  0.f,  1.f,  0.f, 0.f }
	};

	const auto View = XMMatrixLookToLH(Eye, FaceDirections[FaceIdx], FaceUps[FaceIdx]);

	// 90 degrees horizontally and vertically is what makes six faces meet exactly
	const auto Proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, GetShadowNearPlane(), Range);

	return View * Proj;
}

LightConstants PointLight::GetLightConstants() const noexcept
{
	LightConstants Constants	= MakeBaseConstants();
	Constants.Position			= Position;
	Constants.Range				= Range;
	Constants.SourceRadius		= SourceRadius;
	return Constants;
}
