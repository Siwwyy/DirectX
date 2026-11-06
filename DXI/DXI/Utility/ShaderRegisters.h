//Copyright, Damian Andrysiak 2026, All Rights Reserved.

#ifndef D3D12_SHADERREGISTERS_H_INCLUDED
#define D3D12_SHADERREGISTERS_H_INCLUDED

#include "pch.h"

// Which register holds what, for every pass in the engine.
//
// Registers are a per root signature namespace, so two passes can both use b1 without
// colliding. They are still assigned by meaning rather than packed densely, so a reader
// of any shader knows what a slot holds without going to look. A pass that has no use for
// a slot leaves it empty, which is why LightPass starts at b1: it draws a fullscreen
// triangle and has no per object data to put in b0.
//
// shaders/ShaderRegisters.hlsl mirrors these for the HLSL side. The two files have to
// agree, and Verify() below is the one place that says so.
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
