
//Copyright, Damian Andrysiak 2024, All Rights Reserved.

#ifndef PCH_H_INCLUDED
#define PCH_H_INCLUDED


// Libs for linker
//#pragma comment(lib, "d3d12.lib")
//#pragma comment(lib, "D3DCompiler.lib")


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers.
#define _CRT_SECURE_NO_WARNINGS
#define D3DX12_NO_STATE_OBJECT_HELPERS
#endif
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <wincodec.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <cstdint>
#include <wrl/client.h>

#include "d3dx12.h"
#include "d3d12.h"
#include "D3D12Helpers.h"

using namespace DirectX;

#endif //PCH_H_INCLUDED