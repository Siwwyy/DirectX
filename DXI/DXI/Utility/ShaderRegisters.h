//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADERREGISTERS_H_INCLUDED
#define D3D12_SHADERREGISTERS_H_INCLUDED

#include "pch.h"

// Which register holds what. A per root signature namespace, so two passes may both use b1,
// but they are assigned by meaning rather than packed: LightPass starts at b1 because it has
// no per object data. shaders/ShaderRegisters.hlsl mirrors these, Verify() below says so.
namespace ShaderRegister
{
	// b | Constant buffers, numbered by how often the contents change
	constexpr UINT PerObject		= 0;	// Primitive matrices, one per draw
	constexpr UINT PerView			= 1;	// Camera, shadow view, or debug mode, one per pass
	constexpr UINT PerLights		= 2;	// Every light of the scene, one per frame

	// t and s | Not shared. Each pass owns its own table and samplers outright and numbers
	// them from zero, and what they hold has nothing in common between passes: s0 is a
	// linear material sampler in BasePass and a point G-buffer sampler in LightPass. Only
	// the starting slot is worth naming, each pass documents its own contents.
	constexpr UINT TableBase		= 0;
	constexpr UINT SamplerBase		= 0;
}

#endif // D3D12_SHADERREGISTERS_H_INCLUDED
