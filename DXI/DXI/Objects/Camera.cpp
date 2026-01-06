#include "Camera.h"

Camera::Camera(UINT ScreenWidth, UINT ScreenHeight)
	: m_yaw(0.0f)
	, m_pitch(0.0f)
	, m_moveSpeed(5.0f)
	, m_turnSpeed(XM_PIDIV2)
	, CameraMatrices{}
	, KeysPressed{}
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
	const auto HalfFOV			= 45.0f;
	constexpr const auto Near	= 0.01f;
	constexpr const auto Far	= 1000.0f;
	XMMATRIX ProjMat			= XMMatrixPerspectiveFovLH(HalfFOV * (3.14f / 180.0f), 
															static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight), 
															Near, 
															Far); // NOT USED FOR NOW! -> Near and Far are replaced with each other, because of ReverseZ
	XMStoreFloat4x4(&CameraMatrices.ProjMat, ProjMat);

	// set starting camera state
	CameraMatrices.Position		= XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	CameraMatrices.Direction	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	CameraMatrices.Up			= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
}

void Camera::Reset()
{
	m_yaw		= 0.0f;
	m_pitch		= 0.0f;
	KeysPressed = {};
}

void Camera::Update(float ElapsedSeconds) noexcept
{
	// Calculate the move vector in camera space.
	XMFLOAT3 move(0, 0, 0);
	XMFLOAT3 rot(0, 0, 0);

	if (KeysPressed.a)
		move.x -= 1.0f;
	if (KeysPressed.d)
		move.x += 1.0f;
	if (KeysPressed.w)
		move.z += 1.0f;
	if (KeysPressed.s)
		move.z -= 1.0f;

	if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
	{
		XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
		move.x = XMVectorGetX(vector);
		move.z = XMVectorGetZ(vector);
	}

	float moveInterval		= m_moveSpeed * ElapsedSeconds;
	float rotateInterval	= m_turnSpeed * ElapsedSeconds;

	if (KeysPressed.left)
		rot.x -= rotateInterval;	//yaw
	if (KeysPressed.right)
		rot.x += rotateInterval;	//yaw
	if (KeysPressed.up)
		rot.y += rotateInterval;	//pitch
	if (KeysPressed.down)
		rot.y -= rotateInterval;	//pitch

	// Prevent looking too far up or down.
	m_pitch = min(m_pitch, XM_PIDIV4);
	m_pitch = max(-XM_PIDIV4, m_pitch);

	// Move Left, Right, Up, Down
	CameraMatrices.Position.x += move.x * moveInterval;
	CameraMatrices.Position.z += move.z * moveInterval;
	XMMatrixRotationAxis();

	// Roll Pitch Yaw rotation
	CameraMatrices.Direction.x += rot.x;	// Pitch
	CameraMatrices.Direction.y += rot.y;	// Yaw
	//CameraMatrices.Direction.z += m_pitch;	// Roll
}

void Camera::OnKeyDown(WPARAM key)
{
	switch (key)
	{
	case 'W':
		KeysPressed.w = true;
		break;
	case 'A':
		KeysPressed.a = true;
		break;
	case 'S':
		KeysPressed.s = true;
		break;
	case 'D':
		KeysPressed.d = true;
		break;
	case VK_LEFT:
		KeysPressed.left = true;
		break;
	case VK_RIGHT:
		KeysPressed.right = true;
		break;
	case VK_UP:
		KeysPressed.up = true;
		break;
	case VK_DOWN:
		KeysPressed.down = true;
		break;
	case VK_ESCAPE:
		Reset();
		break;
	}
}

void Camera::OnKeyUp(WPARAM key)
{
	switch (key)
	{
	case 'W':
		KeysPressed.w = false;
		break;
	case 'A':
		KeysPressed.a = false;
		break;
	case 'S':
		KeysPressed.s = false;
		break;
	case 'D':
		KeysPressed.d = false;
		break;
	case VK_LEFT:
		KeysPressed.left = false;
		break;
	case VK_RIGHT:
		KeysPressed.right = false;
		break;
	case VK_UP:
		KeysPressed.up = false;
		break;
	case VK_DOWN:
		KeysPressed.down = false;
		break;
	}
}