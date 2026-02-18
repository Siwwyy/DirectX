#include "Camera.h"

static constexpr bool LHCameraCoordinateSystem = true;

Camera::Camera(UINT ScreenWidth, UINT ScreenHeight)
	: Yaw(0.0f)
	, Pitch(0.0f)
	, MoveSpeed(5.0f)
	, TurnSpeed(XM_PIDIV2)
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
	CameraMatrices.Right		= XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	const auto LHMatrix = XMMatrixLookToLH(	XMLoadFloat4(&CameraMatrices.Position),		//	Pos
											XMLoadFloat4(&CameraMatrices.Direction),	// Dir/LookAt/Forward/Target
											XMLoadFloat4(&CameraMatrices.Up));			// Up

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
	Yaw		= 0.0f;
	Pitch		= 0.0f;
	KeysPressed = {};
}

void Camera::Update(float ElapsedSeconds) noexcept
{	
	// Calculate the move vector in camera space.
	const float MoveInterval	= MoveSpeed * ElapsedSeconds;
	const auto MovementVector	= XMVectorSet(MoveInterval, MoveInterval, MoveInterval, 1.f);

	//////////////////////////////////////////////////////////////
	// Movement
	//////////////////////////////////////////////////////////////
	if (KeysPressed.a)
	{
		// cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		//const auto NormalizedCrossProduct	= XMVector3Normalize(XMVector3Cross(XMLoadFloat4(&CameraMatrices.Direction), XMLoadFloat4(&CameraMatrices.Up)));	// glm::normalize(glm::cross(cameraFront, cameraUp))
		const auto MulResult				= XMVectorMultiply(XMLoadFloat4(&CameraMatrices.Right), MovementVector);											// normalized_cross * cameraSpeed;
		const auto FinalPos					= XMVectorSubtract(XMLoadFloat4(&CameraMatrices.Position), MulResult);												// Pos -= MulResult
		XMStoreFloat4(&CameraMatrices.Position, FinalPos);
	}
	if (KeysPressed.d)
	{
		// cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		//const auto NormalizedCrossProduct	= XMVector3Normalize(XMVector3Cross(XMLoadFloat4(&CameraMatrices.Direction), XMLoadFloat4(&CameraMatrices.Up)));	// glm::normalize(glm::cross(cameraFront, cameraUp))
		const auto MulResult				= XMVectorMultiply(XMLoadFloat4(&CameraMatrices.Right), MovementVector);											// normalized_cross * cameraSpeed;
		const auto FinalPos					= XMVectorAdd(XMLoadFloat4(&CameraMatrices.Position), MulResult);													// Pos -= MulResult
		XMStoreFloat4(&CameraMatrices.Position, FinalPos);
	}
	if (KeysPressed.w)
	{
		// cameraPos += cameraSpeed * cameraFront;
		const auto MulResult		= XMVectorMultiply(XMLoadFloat4(&CameraMatrices.Direction), MovementVector);		// cameraSpeed * cameraFront;
		const auto FinalPos			= XMVectorAdd(XMLoadFloat4(&CameraMatrices.Position), MulResult);					// Pos += MulResult
		XMStoreFloat4(&CameraMatrices.Position, FinalPos);
	}
	if (KeysPressed.s)
	{
		// cameraPos -= cameraSpeed * cameraFront;
		const auto MulResult		= XMVectorMultiply(XMLoadFloat4(&CameraMatrices.Direction), MovementVector);		// cameraSpeed * cameraFront;
		const auto FinalPos			= XMVectorSubtract(XMLoadFloat4(&CameraMatrices.Position), MulResult);				// Pos -= MulResult
		XMStoreFloat4(&CameraMatrices.Position, FinalPos);
	}

	//////////////////////////////////////////////////////////////
	// Rotation
	//////////////////////////////////////////////////////////////
	if (KeysPressed.left)
		Yaw += 1.0f;	//yaw
	if (KeysPressed.right)
		Yaw -= 1.0f;	//yaw
	if (KeysPressed.up)
		Pitch += 1.0f;	//pitch
	if (KeysPressed.down)
		Pitch -= 1.0f;	//pitch

	// Make sure to do not make a turn around
	if (Pitch > 89.0f)
		Pitch = 89.0f;
	if (Pitch < -89.0f)
		Pitch = -89.0f;

	//////////////////////////////////////////////////////////////

	// View Matrix contruction
	ConstructViewMatrix(CameraMatrices.Position, CameraMatrices.Direction, CameraMatrices.Up, XMFLOAT4(Helpers::ToRadians(Pitch), Helpers::ToRadians(Yaw), 0.0f, 0.0f));


	//DXLOG("X %f | Z %f | DirX %f | DirZ %f\n", CameraMatrices.Position.x, CameraMatrices.Position.z, CameraMatrices.Direction.x, CameraMatrices.Direction.z)
	//DXLOG("RoX %f | Z %f\n", CameraMatrices.Position.x, CameraMatrices.Position.z)

	auto Matrix = XMMatrixLookToLH(	XMLoadFloat4(&CameraMatrices.Position),		//	Pos
									XMLoadFloat4(&CameraMatrices.Direction),	// Dir/LookAt/Target
									XMLoadFloat4(&CameraMatrices.Up));			// Up
	if constexpr (!LHCameraCoordinateSystem)
	{
		Matrix = XMMatrixLookToRH(	XMLoadFloat4(&CameraMatrices.Position),		//	Pos
									XMLoadFloat4(&CameraMatrices.Direction),	// Dir/LookAt/Target
									XMLoadFloat4(&CameraMatrices.Up));			// Up
	}

	// Store View Mat
	XMStoreFloat4x4(&CameraMatrices.ViewMat, Matrix);
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
	//case VK_ESCAPE:
	//	Reset();
	//	break;
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

	//// If we use Left-Handed Coordinate System, we must negate Camera Direction
	//if constexpr (LHCameraCoordinateSystem)
	//{
	//	XMStoreFloat4(&N, XMVectorNegate(XMLoadFloat4(&N)));
	//}

	// Calculate right vector
	XMStoreFloat4(&U, XMVector3Normalize(XMVector3Cross(XMLoadFloat4(&V), XMLoadFloat4(&N))));

	// Rotate direction vector
	N.x = cosf(RotXYZW.y) * cosf(RotXYZW.x);
	N.y = sinf(RotXYZW.x);
	N.z = sinf(RotXYZW.y) * cosf(RotXYZW.x);


	// Store NVU vectors
	XMStoreFloat4(&CameraMatrices.Direction,	XMLoadFloat4(&N));
	XMStoreFloat4(&CameraMatrices.Up,			XMLoadFloat4(&V));
	XMStoreFloat4(&CameraMatrices.Right,		XMLoadFloat4(&U));

	return;
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
		RotX.m[3][3] = 1;

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
		RotY.m[3][3] = 1;

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
		RotZ.m[3][3] = 1;

		return RotZ;
	};

	////
	// Init Translation Matrix
	XMFLOAT4X4 TranslationMatrix = {};
	// -1* x/y/z because of inverting matrix, simpler to construct than inverting
	// First Row
	TranslationMatrix.m[0][0] = 1;
	TranslationMatrix.m[0][1] = 0;
	TranslationMatrix.m[0][2] = 0;
	TranslationMatrix.m[0][3] = -1.f * 2;

	// Second Row
	TranslationMatrix.m[1][0] = 0;
	TranslationMatrix.m[1][1] = 1;
	TranslationMatrix.m[1][2] = 0;
	TranslationMatrix.m[1][3] = -1.f * 3;

	// Third Row
	TranslationMatrix.m[2][0] = 0;
	TranslationMatrix.m[2][1] = 0;
	TranslationMatrix.m[2][2] = 1;
	TranslationMatrix.m[2][3] = -1.f * 4;

	// Fourth Row
	TranslationMatrix.m[3][0] = 0;
	TranslationMatrix.m[3][1] = 0;
	TranslationMatrix.m[3][2] = 0;
	TranslationMatrix.m[3][3] = 1;

	////
	// Camera Matrix
	XMFLOAT4X4 CameraMatrix = {};
	// First Row
	CameraMatrix.m[0][0] = U.x;
	CameraMatrix.m[1][0] = U.y;
	CameraMatrix.m[2][0] = U.z;
	CameraMatrix.m[3][0] = 0;

	// Second Row
	CameraMatrix.m[0][1] = V.x;
	CameraMatrix.m[1][1] = V.y;
	CameraMatrix.m[2][1] = V.z;
	CameraMatrix.m[3][1] = 0;

	// Third Row
	CameraMatrix.m[0][2] = N.x;
	CameraMatrix.m[1][2] = N.y;
	CameraMatrix.m[2][2] = N.z;
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
	UVNMat.m[0][0] = U.x;
	UVNMat.m[0][1] = U.y;
	UVNMat.m[0][2] = U.z;

	// Second Row
	UVNMat.m[1][0] = V.x;
	UVNMat.m[1][1] = V.y;
	UVNMat.m[1][2] = V.z;

	// Third Row
	UVNMat.m[2][0] = N.x;
	UVNMat.m[2][1] = N.y;
	UVNMat.m[2][2] = N.z;

	// Check determinant
	DXASSERT(XMVector3Equal(DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f), XMMatrixDeterminant((XMLoadFloat3x3(&UVNMat)))) == 0, "Determinant is zero! Linear dependent vectors");

	// Create View Matrix
	const auto Result = XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&CameraMatrix), XMLoadFloat4x4(&TranslationMatrix)));
	//const auto Result = XMMatrixTranspose(* );
	XMStoreFloat4x4(&CameraMatrices.ViewMat, Result);
}