#include "Camera.h"

Camera::Camera(UINT ScreenWidth, UINT ScreenHeight)
{
	/*
		aspectRatio = width/height
		h = 1 / tan(fovy*0.5)
		w = h / aspectRatio
		a = zfar / (zfar - znear)
		b = (-znear * zfar) / (zfar - znear)

		perspective projection matrix
		[w, 0, 0, 0]
		[0, h, 0, 0]
		[0, 0, a, 1]
		[0, 0, b, 0]
	*/

	// build projection and view matrix
	const auto HalfFOV = 45.0f;
	XMMATRIX ProjMat = XMMatrixPerspectiveFovLH(HalfFOV * (3.14f / 180.0f), static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight), 0.1f, 100.0f);
	XMStoreFloat4x4(&CameraMatrices.CameraProjMat, ProjMat);

	// set starting camera state
	CameraMatrices.CameraPosition	= XMFLOAT4(0.0f, 0.0f, -10.0f, 0.0f);
	CameraMatrices.CameraTarget		= XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	CameraMatrices.CameraUp			= XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// build view matrix
	XMVECTOR Pos	= XMLoadFloat4(&CameraMatrices.CameraPosition);
	XMVECTOR Targ	= XMLoadFloat4(&CameraMatrices.CameraTarget);
	XMVECTOR Up		= XMLoadFloat4(&CameraMatrices.CameraUp);
	XMMATRIX LookAt = XMMatrixLookAtLH(Pos, Targ, Up);
	XMStoreFloat4x4(&CameraMatrices.CameraViewMat, LookAt);
}
