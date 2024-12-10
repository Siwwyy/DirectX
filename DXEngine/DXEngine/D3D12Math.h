
//Copyright, Damian Andrysiak 2023, All Rights Reserved.

#ifndef D3D12_MATH_H_INCLUDED
#define D3D12_MATH_H_INCLUDED

#include <DirectXMath.h>
#include "D3D12Helpers.h"

namespace Math
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};


}

#endif // D3D12_MATH_H_INCLUDED