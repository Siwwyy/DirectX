#include "Camera.h"

static constexpr bool LHCameraCoordinateSystem = true;

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

	//// set starting camera state
	CameraMatrices.Position		= XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	CameraMatrices.Direction	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	CameraMatrices.Up			= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	const auto LHMatrix = XMMatrixLookToLH(XMLoadFloat4(&CameraMatrices.Position),		//	Pos
							XMLoadFloat4(&CameraMatrices.Direction),					// Dir/LookAt
							XMLoadFloat4(&CameraMatrices.Up));							// Up

	XMStoreFloat4x4(&CameraMatrices.ViewMat, LHMatrix);

	//// Position Vector
	//CameraMatrices.Position			= XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);

	//// Direction Vector
	//const auto CameraTarget			= XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	//const auto CameraDirection		= XMLoadFloat4(&CameraMatrices.Position) - XMLoadFloat4(&CameraTarget);
	//XMStoreFloat4(&CameraMatrices.Direction, XMVector4Normalize(CameraDirection));
	//// If we use Left-Handed Coordinate System, we must negate Camera Direction
	//if constexpr (LHCameraCoordinateSystem)
	//{
	//	XMStoreFloat4(&CameraMatrices.Direction, XMVector4Normalize(XMVectorNegate(CameraDirection)));
	//}


	//// Right Vector
	//const auto CameraUp				= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	//const auto CameraRight			= XMVector3Cross(XMLoadFloat4(&CameraUp), XMLoadFloat4(&CameraMatrices.Direction));
	//XMStoreFloat4(&CameraMatrices.Right, XMVector4Normalize(CameraRight));

	//// Up Vector
	//XMStoreFloat4(&CameraMatrices.Up, XMVector3Cross(XMLoadFloat4(&CameraMatrices.Direction), XMLoadFloat4(&CameraMatrices.Right)));

	//XMFLOAT4X4 ViewMatrix			= {};
	//// First Row
	//ViewMatrix.m[0][0]				= CameraMatrices.Right.x;
	//ViewMatrix.m[0][1]				= CameraMatrices.Right.y;
	//ViewMatrix.m[0][2]				= CameraMatrices.Right.z;
	//ViewMatrix.m[0][3]				= 0;

	//// Second Row
	//ViewMatrix.m[1][0]				= CameraMatrices.Up.x;
	//ViewMatrix.m[1][1]				= CameraMatrices.Up.y;
	//ViewMatrix.m[1][2]				= CameraMatrices.Up.z;
	//ViewMatrix.m[1][3]				= 0;

	//// Third Row
	//ViewMatrix.m[2][0]				= CameraMatrices.Direction.x;
	//ViewMatrix.m[2][1]				= CameraMatrices.Direction.y;
	//ViewMatrix.m[2][2]				= CameraMatrices.Direction.z;
	//ViewMatrix.m[2][3]				= 0;

	//// Fourth Row
	//ViewMatrix.m[3][0]				= 0;
	//ViewMatrix.m[3][1]				= 0;
	//ViewMatrix.m[3][2]				= 0;
	//ViewMatrix.m[3][3]				= 1;

	//XMFLOAT4X4 TranslationMatrix = {};
	//// First Row
	//TranslationMatrix.m[0][0] = 1;
	//TranslationMatrix.m[0][1] = 0;
	//TranslationMatrix.m[0][2] = 0;
	//TranslationMatrix.m[0][3] = -1.f * CameraMatrices.Position.x;

	//// Second Row
	//TranslationMatrix.m[1][0] = 0;
	//TranslationMatrix.m[1][1] = 1;
	//TranslationMatrix.m[1][2] = 0;
	//TranslationMatrix.m[1][3] = -1.f * CameraMatrices.Position.y;

	//// Third Row
	//TranslationMatrix.m[2][0] = 0;
	//TranslationMatrix.m[2][1] = 0;
	//TranslationMatrix.m[2][2] = 1;
	//TranslationMatrix.m[2][3] = -1.f * CameraMatrices.Position.z;

	//// Fourth Row
	//TranslationMatrix.m[3][0] = 0;
	//TranslationMatrix.m[3][1] = 0;
	//TranslationMatrix.m[3][2] = 0;
	//TranslationMatrix.m[3][3] = 1;

	//// Store ViewMat
	//XMStoreFloat4x4(&CameraMatrices.ViewMat, XMMatrixTranspose(XMLoadFloat4x4(&ViewMatrix) * XMLoadFloat4x4(&TranslationMatrix)));
	//const auto CompareMat = XMLoadFloat4x4(&CameraMatrices.ViewMat);
	//const auto Result = XMLoadFloat4x4(&CameraMatrices.ViewMat) - ABC;
	//const auto Result2 = XMLoadFloat4x4(&CameraMatrices.ViewMat) - ABC;
	//ConstructViewMatrix(CameraMatrices.Position, CameraMatrices.Direction, XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
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
	XMFLOAT3 rot(0, 0, 0); //yaw, pitch, roll

	//////////////////////////////////////////////////////////////
	// New
	//////////////////////////////////////////////////////////////
	if (KeysPressed.a)
	{
		move.x -= 1.0f;
	}
	if (KeysPressed.d)
	{
		move.x += 1.0f;
	}
	if (KeysPressed.w)
	{
		move.z += 1.0f;
	}
	if (KeysPressed.s)
	{
		move.z -= 1.0f;
	}

	// Normalization
	if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
	{
		XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
		move.x = XMVectorGetX(vector);
		move.z = XMVectorGetZ(vector);
	}

	//
	float MoveInterval = m_moveSpeed * ElapsedSeconds;
	// Move Left, Right, Backward, Forward
	CameraMatrices.Position.x += move.x * MoveInterval;
	CameraMatrices.Position.z += move.z * MoveInterval;
	//////////////////////////////////////////////////////////////

	// Rotation

	if (KeysPressed.left)
		rot.y += 1.0f;	//yaw
	if (KeysPressed.right)
		rot.y -= 1.0f;	//yaw
	if (KeysPressed.up)
		rot.x += 1.0f;	//pitch
	if (KeysPressed.down)
		rot.x -= 1.0f;	//pitch


	ConstructViewMatrix(CameraMatrices.Position, CameraMatrices.Direction, CameraMatrices.Up, XMFLOAT4(rot.x, rot.y, 0.0f, 0.0f));


	DXLOG("X %f | Z %f | DirX %f | DirZ %f\n", CameraMatrices.Position.x, CameraMatrices.Position.z, CameraMatrices.Direction.x, CameraMatrices.Direction.z)
	//DXLOG("RoX %f | Z %f\n", CameraMatrices.Position.x, CameraMatrices.Position.z)
	//if (KeysPressed.a)
	//	move.x -= 1.0f;
	//if (KeysPressed.d)
	//	move.x += 1.0f;
	//if (KeysPressed.w)
	//	move.z += 1.0f;
	//if (KeysPressed.s)
	//	move.z -= 1.0f;

	//if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
	//{
	//	XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
	//	move.x = XMVectorGetX(vector);
	//	move.z = XMVectorGetZ(vector);
	//}

	//float moveInterval		= m_moveSpeed * ElapsedSeconds;
	//float rotateInterval	= m_turnSpeed * ElapsedSeconds;

	//if (KeysPressed.left)
	//	rot.x -= 1.0f;	//yaw
	//if (KeysPressed.right)
	//	rot.x += 1.0f;	//yaw
	//if (KeysPressed.up)
	//	rot.y += 1.0f;	//pitch
	//if (KeysPressed.down)
	//	rot.y -= 1.0f;	//pitch

	//// Prevent looking too far up or down.
	//m_pitch = min(m_pitch, XM_PIDIV4);
	//m_pitch = max(-XM_PIDIV4, m_pitch);

	//// Move Left, Right, Up, Down
	//CameraMatrices.Position.x += move.x * moveInterval;
	//CameraMatrices.Position.z += move.z * moveInterval;




	//// Convert to radians
	//rot.x = ToRadians(rot.x);
	//rot.y = ToRadians(rot.y);
	//rot.z = ToRadians(rot.z);

	//// Roll Pitch Yaw rotation
	////CameraMatrices.Direction.x += cosf(rot.x * (3.14 / 180.f)) * cosf(rot.y * (3.14 / 180.f)) * rotateInterval;			// Yaw
	////CameraMatrices.Direction.z += sinf(rot.x * (3.14 / 180.f)) * cosf(rot.y * (3.14 / 180.f)) * rotateInterval;		// Yaw
	////CameraMatrices.Direction.y += sinf(rot.y * (3.14 / 180.f));											// Pitch

	////XMStoreFloat4(&CameraMatrices.Direction, XMVector4Normalize(XMLoadFloat4(&CameraMatrices.Direction)));
	//ConstructViewMatrix(CameraMatrices.Position, CameraMatrices.Direction, XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));


	//const auto LHMatrix = XMMatrixLookToLH(XMLoadFloat4(&CameraMatrices.Position),		//	Pos
	//	XMLoadFloat4(&CameraMatrices.Direction),					// Dir/LookAt
	//	XMLoadFloat4(&CameraMatrices.Up));							// Up

	//XMStoreFloat4x4(&CameraMatrices.ViewMat, LHMatrix);
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

// overload "<<"
std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v)
{
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";
	return os;
}

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMMATRIX m)
{
	for (int x = 0; x < 4; ++x)
	{
		os << XMVectorGetX(m.r[x]) << "\t";
		os << XMVectorGetY(m.r[x]) << "\t";
		os << XMVectorGetZ(m.r[x]) << "\t";
		os << XMVectorGetW(m.r[x]) << "\t";
		os << std::endl;
	}

	return os;
}

void Camera::ConstructViewMatrix(const XMFLOAT4 Pos, const XMFLOAT4 Dir, const XMFLOAT4 Up, const XMFLOAT4 RotXYZW)
{
	// U V N
	// U -> Right vector
	// N -> Direction/Target vector
	// V -> Up vector
	auto U = XMFLOAT4{};
	auto V = XMFLOAT4{};
	auto N = XMFLOAT4{};

	// Normalize
	XMStoreFloat4(&N, XMVector3Normalize(XMLoadFloat4(&Dir)));
	XMStoreFloat4(&V, XMVector3Normalize(XMLoadFloat4(&Up)));

	// If we use Left-Handed Coordinate System, we must negate Camera Direction
	if constexpr (LHCameraCoordinateSystem)
	{
		XMStoreFloat4(&N, XMVectorNegate(XMLoadFloat4(&N)));
	}

	// Calculate right vector
	XMStoreFloat4(&U, XMVector3Cross(XMLoadFloat4(&V), XMLoadFloat4(&N)));

	// Rotation
	// RotX | Pitch
	auto RotateAroundXAxis = [](float ThetaRadians)
	{
		XMFLOAT4X4 RotX = {};
		RotX.m[0][0] = 1;
		RotX.m[0][1] = 0;
		RotX.m[0][2] = 0;
		RotX.m[0][3] = 0;

		// Second Row
		RotX.m[1][0] = 0;
		RotX.m[1][1] = cosf(ThetaRadians);
		RotX.m[1][2] = -1.f * sinf(ThetaRadians);
		RotX.m[1][3] = 0;

		// Third Row
		RotX.m[2][0] = 0;
		RotX.m[2][1] = sinf(ThetaRadians);
		RotX.m[2][2] = cosf(ThetaRadians);
		RotX.m[2][3] = 0;

		// Fourth Row
		RotX.m[3][0] = 0;
		RotX.m[3][1] = 0;
		RotX.m[3][2] = 0;
		RotX.m[3][3] = 0;

		return RotX;
	};

	// RotY | Yaw
	auto RotateAroundYAxis = [](float ThetaRadians)
	{
		XMFLOAT4X4 RotY = {};
		// First Row
		RotY.m[0][0] = cosf(ThetaRadians);
		RotY.m[0][1] = 0;
		RotY.m[0][2] = sinf(ThetaRadians);
		RotY.m[0][3] = 0;

		// Second Row
		RotY.m[1][0] = 0;
		RotY.m[1][1] = 1;
		RotY.m[1][2] = 0;
		RotY.m[1][3] = 0;

		// Third Row
		RotY.m[2][0] = -1.f * sinf(ThetaRadians);
		RotY.m[2][1] = 0;
		RotY.m[2][2] = cosf(ThetaRadians);
		RotY.m[2][3] = 0;

		// Fourth Row
		RotY.m[3][0] = 0;
		RotY.m[3][1] = 0;
		RotY.m[3][2] = 0;
		RotY.m[3][3] = 0;

		return RotY;
	};


	// RotZ | Roll
	auto RotateAroundZAxis = [](float ThetaRadians)
	{
		XMFLOAT4X4 RotZ = {};
		// First Row
		RotZ.m[0][0] = cosf(ThetaRadians);
		RotZ.m[0][1] = -1.f * sinf(ThetaRadians);
		RotZ.m[0][2] = 0;
		RotZ.m[0][3] = 0;

		// Second Row
		RotZ.m[1][0] = sinf(ThetaRadians);
		RotZ.m[1][1] = cosf(ThetaRadians);
		RotZ.m[1][2] = 0;
		RotZ.m[1][3] = 0;

		// Third Row
		RotZ.m[2][0] = 0;
		RotZ.m[2][1] = 0;
		RotZ.m[2][2] = 1;
		RotZ.m[2][3] = 0;

		// Fourth Row
		RotZ.m[3][0] = 0;
		RotZ.m[3][1] = 0;
		RotZ.m[3][2] = 0;
		RotZ.m[3][3] = 0;

		return RotZ;
	};


	// Rotate around Yaw Y, we rotate Z axis
	//const auto RotatorY = RotateAroundYAxis(Helpers::ToRadians(RotXYZW.y));
	const auto RotatorX = RotateAroundXAxis(Helpers::ToRadians(RotXYZW.x));
	const auto RotatorY = RotateAroundYAxis(Helpers::ToRadians(RotXYZW.y));
	const auto RotatorZ = RotateAroundZAxis(Helpers::ToRadians(0));
	XMFLOAT4 Nz = {};
	XMStoreFloat4(&Nz, XMVector4Transform(XMLoadFloat4(&N), XMLoadFloat4x4(&RotatorY)));
	XMStoreFloat4(&Nz, XMVector4Transform(XMLoadFloat4(&Nz), XMLoadFloat4x4(&RotatorY)));
	XMStoreFloat4(&Nz, XMVector4Transform(XMLoadFloat4(&Nz), XMLoadFloat4x4(&RotatorY)));

	// Rotate around Yaw Y, we rotate X axis

	XMFLOAT4 Ux = U;
	XMStoreFloat4(&Ux, XMVector4Transform(XMLoadFloat4(&U), XMLoadFloat4x4(&RotatorX)));
	XMStoreFloat4(&Ux, XMVector4Transform(XMLoadFloat4(&Ux), XMLoadFloat4x4(&RotatorY)));
	XMStoreFloat4(&Ux, XMVector4Transform(XMLoadFloat4(&Ux), XMLoadFloat4x4(&RotatorZ)));

	// Rotate around Yaw Y, we rotate X axis
	XMFLOAT4 Vy = {};
	XMStoreFloat4(&Vy, XMVector4Transform(XMLoadFloat4(&V), XMLoadFloat4x4(&RotatorX)));
	XMStoreFloat4(&Vy, XMVector4Transform(XMLoadFloat4(&Vy), XMLoadFloat4x4(&RotatorY)));
	XMStoreFloat4(&Vy, XMVector4Transform(XMLoadFloat4(&Vy), XMLoadFloat4x4(&RotatorZ)));


	XMStoreFloat4(&CameraMatrices.Direction, XMVector3Normalize(XMLoadFloat4(&Nz)));
	XMStoreFloat4(&CameraMatrices.Up, XMVector3Normalize(XMLoadFloat4(&Vy)));

	////
	// Init Translation Matrix
	XMFLOAT4X4 TranslationMatrix = {};
	// -1* x/y/z because of inverting matrix, simpler to construct than inverting
	// First Row
	TranslationMatrix.m[0][0] = 1;
	TranslationMatrix.m[0][1] = 0;
	TranslationMatrix.m[0][2] = 0;
	TranslationMatrix.m[0][3] = -1.f * CameraMatrices.Position.x;

	// Second Row
	TranslationMatrix.m[1][0] = 0;
	TranslationMatrix.m[1][1] = 1;
	TranslationMatrix.m[1][2] = 0;
	TranslationMatrix.m[1][3] = -1.f * CameraMatrices.Position.y;

	// Third Row
	TranslationMatrix.m[2][0] = 0;
	TranslationMatrix.m[2][1] = 0;
	TranslationMatrix.m[2][2] = 1;
	TranslationMatrix.m[2][3] = -1.f * CameraMatrices.Position.z;

	// Fourth Row
	TranslationMatrix.m[3][0] = 0;
	TranslationMatrix.m[3][1] = 0;
	TranslationMatrix.m[3][2] = 0;
	TranslationMatrix.m[3][3] = 1;

	////
	// Camera Matrix
	XMFLOAT4X4 CameraMatrix = {};
	// First Row
	CameraMatrix.m[0][0] = Ux.x;
	CameraMatrix.m[1][0] = Ux.y;
	CameraMatrix.m[2][0] = Ux.z;
	CameraMatrix.m[3][0] = 0;

	// Second Row
	CameraMatrix.m[0][1] = Vy.x;
	CameraMatrix.m[1][1] = Vy.y;
	CameraMatrix.m[2][1] = Vy.z;
	CameraMatrix.m[3][1] = 0;

	// Third Row
	CameraMatrix.m[0][2] = Nz.x;
	CameraMatrix.m[1][2] = Nz.y;
	CameraMatrix.m[2][2] = Nz.z;
	CameraMatrix.m[3][2] = 0;

	// Fourth Row
	CameraMatrix.m[0][3] = 0;
	CameraMatrix.m[1][3] = 0;
	CameraMatrix.m[2][3] = 0;
	CameraMatrix.m[3][3] = 1;

	// Above, we created Camera Coordinate System Matrix, now, we can inverse it, i.e., Transpose
	// Because vectors are unit length and orthogonal i.e., linearily independent, but double check!
	// Calculate Determinant of 3x3 matrix which consists of three vectors: U,V,C
	XMFLOAT3X3 UVNMat;

	// First Row
	UVNMat.m[0][0] = Ux.x;
	UVNMat.m[0][1] = Ux.y;
	UVNMat.m[0][2] = Ux.z;

	// Second Row
	UVNMat.m[1][0] = Vy.x;
	UVNMat.m[1][1] = Vy.y;
	UVNMat.m[1][2] = Vy.z;

	// Third Row
	UVNMat.m[2][0] = Nz.x;
	UVNMat.m[2][1] = Nz.y;
	UVNMat.m[2][2] = Nz.z;

	// Check determinant
	DXASSERT(XMVector3Equal(DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f), XMMatrixDeterminant((XMLoadFloat3x3(&UVNMat)))) == 0, "Determinant is zero! Linear dependent vectors");


	// Create View Matrix
	XMStoreFloat4x4(&CameraMatrices.ViewMat, XMMatrixTranspose(XMLoadFloat3x3(&UVNMat) * XMLoadFloat4x4(&TranslationMatrix)));

	//// Position Vector
	//CameraMatrices.Position = Pos;

	//// Direction Vector
	//const auto CameraTarget = Dir;
	//const auto CameraDirection = XMLoadFloat4(&CameraMatrices.Position) - XMLoadFloat4(&CameraTarget);
	//XMStoreFloat4(&CameraMatrices.Direction, XMVector4Normalize(CameraDirection));
	//// If we use Left-Handed Coordinate System, we must negate Camera Direction
	//if constexpr (LHCameraCoordinateSystem)
	//{
	//	XMStoreFloat4(&CameraMatrices.Direction, XMVector4Normalize(XMVectorNegate(CameraDirection)));
	//}


	//// Right Vector
	//const auto CameraUp = Up;
	//const auto CameraRight = XMVector3Cross(XMLoadFloat4(&CameraUp), XMLoadFloat4(&CameraMatrices.Direction));
	//XMStoreFloat4(&CameraMatrices.Right, XMVector4Normalize(CameraRight));

	//// Up Vector
	//XMStoreFloat4(&CameraMatrices.Up, XMVector3Cross(XMLoadFloat4(&CameraMatrices.Direction), XMLoadFloat4(&CameraMatrices.Right)));

	//XMFLOAT4X4 ViewMatrix = {};
	//// First Row
	//ViewMatrix.m[0][0] = CameraMatrices.Right.x;
	//ViewMatrix.m[0][1] = CameraMatrices.Right.y;
	//ViewMatrix.m[0][2] = CameraMatrices.Right.z;
	//ViewMatrix.m[0][3] = 0;

	//// Second Row
	//ViewMatrix.m[1][0] = CameraMatrices.Up.x;
	//ViewMatrix.m[1][1] = CameraMatrices.Up.y;
	//ViewMatrix.m[1][2] = CameraMatrices.Up.z;
	//ViewMatrix.m[1][3] = 0;

	//// Third Row
	//ViewMatrix.m[2][0] = CameraMatrices.Direction.x;
	//ViewMatrix.m[2][1] = CameraMatrices.Direction.y;
	//ViewMatrix.m[2][2] = CameraMatrices.Direction.z;
	//ViewMatrix.m[2][3] = 0;

	//// Fourth Row
	//ViewMatrix.m[3][0] = 0;
	//ViewMatrix.m[3][1] = 0;
	//ViewMatrix.m[3][2] = 0;
	//ViewMatrix.m[3][3] = 1;

	//XMFLOAT4X4 TranslationMatrix = {};
	//// -1* x/y/z because of inverting matrix, simpler to construct than inverting
	//// First Row
	//TranslationMatrix.m[0][0] = 1;
	//TranslationMatrix.m[0][1] = 0;
	//TranslationMatrix.m[0][2] = 0;
	//TranslationMatrix.m[0][3] = -1.f * CameraMatrices.Position.x;

	//// Second Row
	//TranslationMatrix.m[1][0] = 0;
	//TranslationMatrix.m[1][1] = 1;
	//TranslationMatrix.m[1][2] = 0;
	//TranslationMatrix.m[1][3] = -1.f * CameraMatrices.Position.y;

	//// Third Row
	//TranslationMatrix.m[2][0] = 0;
	//TranslationMatrix.m[2][1] = 0;
	//TranslationMatrix.m[2][2] = 1;
	//TranslationMatrix.m[2][3] = -1.f * CameraMatrices.Position.z;

	//// Fourth Row
	//TranslationMatrix.m[3][0] = 0;
	//TranslationMatrix.m[3][1] = 0;
	//TranslationMatrix.m[3][2] = 0;
	//TranslationMatrix.m[3][3] = 1;

	//// Store ViewMat
	//XMStoreFloat4x4(&CameraMatrices.ViewMat, XMMatrixTranspose(XMLoadFloat4x4(&ViewMatrix) * XMLoadFloat4x4(&TranslationMatrix)));
}