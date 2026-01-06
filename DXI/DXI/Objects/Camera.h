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

	// Ctors
	Camera()	= default;
	Camera(UINT ScreenWidth, UINT ScreenHeight);
	~Camera()	= default;

	// Logic
	void Reset();
	void Update(float ElapsedSeconds) noexcept;

	// Movement
	void OnKeyDown(WPARAM key);
	void OnKeyUp(WPARAM key);

	//// Setters
	//void SetPosVector(const XMFLOAT4 Vector)
	//{
	//	// set starting camera state
	//	CameraMatrices.Position = Vector;

	//	// build view matrix
	//	XMVECTOR Pos	= XMLoadFloat4(&CameraMatrices.Position);
	//	XMVECTOR Targ	= XMLoadFloat4(&CameraMatrices.Direction);
	//	XMVECTOR Up		= XMLoadFloat4(&CameraMatrices.Up);
	//	XMMATRIX LookAt = XMMatrixLookAtLH(Pos, Targ, Up);
	//	XMStoreFloat4x4(&CameraMatrices.ViewMat, LookAt);
	//}

	// Getters
	_NODISCARD inline const auto GetViewMatrix()
	{
		return XMMatrixLookToLH(XMLoadFloat4(&CameraMatrices.Position),		//	Pos
				XMLoadFloat4(&CameraMatrices.Direction),					// Dir/LookAt
				XMLoadFloat4(&CameraMatrices.Up));							// Up
	}
	_NODISCARD inline const auto GetProjMatrix() const
	{
		return XMLoadFloat4x4(&CameraMatrices.ProjMat);						// Proj mat
	}

private:

	struct KeysPressed
	{
		bool w;
		bool a;
		bool s;
		bool d;

		bool left;
		bool right;
		bool up;
		bool down;
	};

	//
	float m_yaw;							// Relative to the +z axis.
	float m_pitch;							// Relative to the xz plane.
	float m_moveSpeed = 1.f;				// Speed at which the camera moves, in units per second.
	float m_turnSpeed = 1.f;				// Speed at which the camera turns, in radians per second.

	// Matrices
	CameraMatrices	CameraMatrices;

	// Keys
	KeysPressed KeysPressed;

};

#endif // D3D12_CAMERA_H_INCLUDED