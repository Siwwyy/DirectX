// Copyright, Damian Andrysiak 2026, All Rights Reserved.

// HLSL half of the register convention. Mirrors ShaderRegisters.h, which carries the same
// slots as integers for the root signature side.
//
// HLSL cannot build a register token out of a number, so these are spelled out rather
// than shared with the C++ header. Changing one means changing the other.

// b | Constant buffers, numbered by how often the contents change
#define CB_PER_OBJECT       b0      // Primitive matrices, one per draw
#define CB_PER_VIEW         b1      // Camera, shadow view, or debug mode, one per pass
#define CB_PER_LIGHTS       b2      // Every light of the scene, one per frame

// Textures and samplers are deliberately absent. Each pass owns its own and numbers them
// from zero, and the contents do not line up between passes, so there is nothing to share.
