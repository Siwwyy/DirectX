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

	// Rotation | Takes how many pixels the cursor travelled since the last message
	void OnMouseMove(const int DeltaX, const int DeltaY) noexcept;

	// Setters
	void SetMouseSensitivity(const float Sensitivity) noexcept
	{
		MouseSensitivity = Sensitivity;
	}

	// Getters
	_NODISCARD inline auto GetMouseSensitivity() const noexcept
	{
		return MouseSensitivity;
	}
	_NODISCARD inline const auto GetViewMatrix() const
	{
		return XMLoadFloat4x4(&CameraMatrices.ViewMat);						// View mat
	}
	_NODISCARD inline const auto GetProjMatrix() const
	{
		return XMLoadFloat4x4(&CameraMatrices.ProjMat);						// Proj mat
	}
	_NODISCARD inline const auto& GetPosition() const
	{
		return CameraMatrices.Position;										// World space position (eye)
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
	float MouseSensitivity = 0.2f;			// Degrees of rotation per pixel of cursor movement.

	// Matrices
	CameraMatrices	CameraMatrices;

	// Keys
	KeysPressed KeysPressed;

};

#endif // D3D12_CAMERA_H_INCLUDED