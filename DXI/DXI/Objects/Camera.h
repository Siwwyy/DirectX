//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_CAMERA_H_INCLUDED
#define D3D12_CAMERA_H_INCLUDED


#include "../pch.h"
#include "../D3D12Helpers.h"
#include "../D3D12Math.h"

class Camera
{
	DISABLE_COPY(Camera)

public:

	Camera()	= default;
	~Camera()	= default;

	Camera(UINT ScreenWidth, UINT ScreenHeight);

	void SetPosVector(const XMFLOAT4 Vector)
	{
		// set starting camera state
		CameraMatrices.CameraPosition = Vector;

		// build view matrix
		XMVECTOR Pos	= XMLoadFloat4(&CameraMatrices.CameraPosition);
		XMVECTOR Targ	= XMLoadFloat4(&CameraMatrices.CameraTarget);
		XMVECTOR Up		= XMLoadFloat4(&CameraMatrices.CameraUp);
		XMMATRIX LookAt = XMMatrixLookAtLH(Pos, Targ, Up);
		XMStoreFloat4x4(&CameraMatrices.CameraViewMat, LookAt);
	}

	_NODISCARD inline const auto GetViewMatrix() const
	{
		return CameraMatrices.CameraViewMat;
	}
	_NODISCARD inline const auto GetProjMatrix() const
	{
		return CameraMatrices.CameraProjMat;
	}

private:
	CameraMatrices	CameraMatrices;
};


#endif // D3D12_CAMERA_H_INCLUDED