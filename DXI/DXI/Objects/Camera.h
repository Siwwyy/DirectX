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

	// Getters
	_NODISCARD inline const auto GetViewMatrix()
	{
		return XMLoadFloat4x4(&CameraMatrices.ViewMat);						// View mat
	}
	_NODISCARD inline const auto GetProjMatrix() const
	{
		return XMLoadFloat4x4(&CameraMatrices.ProjMat);						// Proj mat
	}

private:

	void ConstructViewMatrix(const XMFLOAT4 Pos, 
							const XMFLOAT4 Dir,
							const XMFLOAT4 Up, 
							const XMFLOAT4 RotXYZW);
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
	float Yaw;								// Relative to the y plane.
	float Pitch;							// Relative to the xz plane.
	float MoveSpeed;						// Speed at which the camera moves, in units per second.
	float TurnSpeed;						// Speed at which the camera turns, in radians per second.

	// Matrices
	CameraMatrices	CameraMatrices;

	// Keys
	KeysPressed KeysPressed;

};

#endif // D3D12_CAMERA_H_INCLUDED