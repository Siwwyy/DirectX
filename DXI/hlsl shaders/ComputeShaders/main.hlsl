// Simple compute rasterizer (corrected)
// Resources:
//  t0: StructuredBuffer<float3> g_Vertices
//  t1: StructuredBuffer<uint3>  g_Triangles
//  t2: StructuredBuffer<float4>  g_VertexColors
//  b0: CameraCB { float4x4 ViewProj; uint2 ScreenSize; }
//  u0: RWStructuredBuffer<float4> g_Output
//
// NOTE: Verify matrix multiplication order with your CPU:
// - If you upload a column-major ViewProj and intend M * v, use mul(ViewProj, float4(pos,1))
// - If you upload row-major or transposed matrices, change the mul order accordingly.

cbuffer CameraCB : register(b0)
{
    float4x4 ViewProj; // set from CPU; match multiply order below
    uint2    ScreenSize;
    uint     _pad0;
};

StructuredBuffer<float3> g_Vertices     : register(t0);
StructuredBuffer<uint3>  g_Triangles    : register(t1);
StructuredBuffer<float4> g_VertexColors : register(t2);

RWStructuredBuffer<float4> g_Output     : register(u0);

// edge function: cross(b - a, c - a)
static float edgeFunction(float2 a, float2 b, float2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Project world position -> screen XY and return clip.w and ndc.z
static void ProjectToScreen(float3 pos, out float2 screenXY, out float ndcZ, out float clipW)
{
    // Use matrix * vector (column-major) multiplication
    float4 clip = mul(ViewProj, float4(pos, 1.0f));
    clipW = clip.w;
    float invW = 1.0f / clipW;
    float3 ndc = clip.xyz * invW;
    // Map NDC [-1,1] to pixel coordinates (pixel center at +0.5)
    screenXY.x = (ndc.x * 0.5f + 0.5f) * (float)ScreenSize.x;
    // flip Y if your render target origin requires it; this example assumes top-left origin
    screenXY.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)ScreenSize.y;
    ndcZ = ndc.z; // depth in NDC space
}

[numthreads(8,8,1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint px = dispatchThreadID.x;
    uint py = dispatchThreadID.y;

    if (px >= ScreenSize.x || py >= ScreenSize.y) return;

    float2 p = float2(px + 0.5f, py + 0.5f);

    float bestDepth = 1e9; // larger is farther (choose appropriate space)
    float4 outColor = float4(0,0,0,1);

    uint triCount = g_Triangles.Length;
    for (uint ti = 0; ti < triCount; ++ti)
    {
        uint3 idx = g_Triangles[ti];

        float3 v0 = g_Vertices[idx.x];
        float3 v1 = g_Vertices[idx.y];
        float3 v2 = g_Vertices[idx.z];

        float2 s0, s1, s2;
        float nz0, nz1, nz2;
        float w0_clip, w1_clip, w2_clip;

        ProjectToScreen(v0, s0, nz0, w0_clip);
        ProjectToScreen(v1, s1, nz1, w1_clip);
        ProjectToScreen(v2, s2, nz2, w2_clip);

        // bounding box test
        float minX = min(min(s0.x, s1.x), s2.x);
        float maxX = max(max(s0.x, s1.x), s2.x);
        float minY = min(min(s0.y, s1.y), s2.y);
        float maxY = max(max(s0.y, s1.y), s2.y);

        if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY) continue;

        // compute area and skip degenerate triangles
        float area = edgeFunction(s0, s1, s2);
        if (abs(area) < 1e-6f) continue;

        // barycentric weights
        float w0 = edgeFunction(s1, s2, p) / area;
        float w1 = edgeFunction(s2, s0, p) / area;
        float w2 = edgeFunction(s0, s1, p) / area;

        // inside test (use >=0 or >0 depending on fill rule)
        if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
        {
            // perspective-correct interpolation:
            float iw0 = 1.0f / w0_clip;
            float iw1 = 1.0f / w1_clip;
            float iw2 = 1.0f / w2_clip;

            // denominator (sum of barycentric*1/w)
            float denom = w0 * iw0 + w1 * iw1 + w2 * iw2;

            // interpolate NDC.z (z/w) using (sum w_i * (z_i / w_i)) / denom
            float ndcZInterp = (w0 * (nz0) * iw0 + w1 * (nz1) * iw1 + w2 * (nz2) * iw2) / denom;

            // Use ndcZInterp (or convert to view-space depth if needed)
            // Here we treat a smaller value as closer if your NDC mapping is such; adjust comparison accordingly
            if (ndcZInterp < bestDepth)
            {
                bestDepth = ndcZInterp;

                // interpolate color with perspective correction:
                float4 c0 = g_VertexColors[idx.x];
                float4 c1 = g_VertexColors[idx.y];
                float4 c2 = g_VertexColors[idx.z];

                float4 colorNumer = w0 * (c0 * iw0) + w1 * (c1 * iw1) + w2 * (c2 * iw2);
                float4 finalColor = colorNumer / denom;
                outColor = finalColor;
            }
        }
    }

    uint outIndex = py * ScreenSize.x + px;
    g_Output[outIndex] = outColor;
}