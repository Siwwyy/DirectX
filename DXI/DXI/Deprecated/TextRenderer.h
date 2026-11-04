//Copyright, Damian Andrysiak 2025, All Rights Reserved.

#ifndef D3D12_TEXT_RENDERER_H_INCLUDED
#define D3D12_TEXT_RENDERER_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"


struct TextVertex {
    TextVertex(float r, float g, float b, float a, float u, float v, float tw, float th, float x, float y, float w, float h) : color(r, g, b, a), texCoord(u, v, tw, th), pos(x, y, w, h) {}
    XMFLOAT4 pos;
    XMFLOAT4 texCoord;
    XMFLOAT4 color;
};

#endif // D3D12_TEXT_RENDERER_H_INCLUDED