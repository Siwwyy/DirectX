// Simple HLSL compute shader
// - 3 SRVs (Texture2D) at t0,t1,t2
// - 1 UAV (RWTexture2D) at u0
// - Constant buffer (b0) containing Projection and View matrices, also ScreenSize

cbuffer CameraCB : register(b0)
{
    float4x4    ViewMatrix;
    float4x4    ProjeMatrix;
    float4x4    ViewProjMatrix;
    float4      ScreenSize;
};

// input SRVs
StructuredBuffer<float4>        SRV0 : register(t0); // color
StructuredBuffer<float4>        SRV1 : register(t1); // depth/stencil
StructuredBuffer<float4>        SRV2 : register(t2); // vertices
StructuredBuffer<float4>        SRV3 : register(t3); // color per vertex
StructuredBuffer<uint3>         SRV4 : register(t4); // indices per triangle
// output UAV
RWStructuredBuffer<float4>      UAV0 : register(u0);

//// thread group size (adjust to your dispatch)
//// 8, VERTICES_COUNT, 1
//[numthreads(8, VERTICES_COUNT, 1)]
//void CSMain(uint3 GroupThreadID : SV_GroupThreadID,
//            uint3 DispatchThreadID : SV_DispatchThreadID)
//{
//    // Indexes
//    uint2 xy                = DispatchThreadID.xy;
//    uint flatten_idx        = xy.y * (uint)ScreenSize.x + xy.x;
//    uint vertex_buffer_idx  = GroupThreadID.y;
//
//	// Projection to Clip position
//    const float4 VertexProjPos  = mul(SRV2[vertex_buffer_idx], ViewProjMatrix);
//    const float4 Color          = SRV3[vertex_buffer_idx];
//
//	// to NDC (Normalized Device Coordinates)
//	const float4 VertexNDCPos   = VertexProjPos / VertexProjPos.w; // Normalize to NDC
//
//    // Output the first SRV to the UAV (for testing)
//    //UAV0[flatten_idx] = float4(255,0,0,255);
//    UAV0[flatten_idx] = SRV0[flatten_idx] + uint4(Color.xyzw * 255.0f);
//}

// helpers
// edge function: cross(b - a, c - a)
static float edgeFunction(float2 a, float2 b, float2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Project world position -> screen XY and return clip.w and ndc.z
static void ProjectToScreen(float3 pos, out float2 screenXY, out float ndcZ, out float clipW)
{
    // Use matrix * vector (column-major) multiplication
    float4 clip = mul(ViewProjMatrix, float4(pos, 1.0f));
    clipW = clip.w;
    float invW = 1.0f / clipW;
    float3 ndc = clip.xyz * invW;
    // Map NDC [-1,1] to pixel coordinates (pixel center at +0.5)
    screenXY.x = (ndc.x * 0.5f + 0.5f) * (float)ScreenSize.x;
    // flip Y if your render target origin requires it; this example assumes top-left origin
    screenXY.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)ScreenSize.y;
    ndcZ = ndc.z; // depth in NDC space
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint px = dispatchThreadID.x;
    uint py = dispatchThreadID.y;

    if (px >= ScreenSize.x || py >= ScreenSize.y)
        return;

    // pixel center in screen space
    float2 p = float2(px + 0.5f, py + 0.5f);

    // best match
    float bestDepth = 1e9; // large value (far)
    float4 outColor = float4(0, 0, 0, 1);

    // Iterate all triangles (brute-force)
    uint triCount = 2;
    //SRV4.GetDimensions(triCount);
    for (uint ti = 0; ti < triCount; ++ti)
    {
        uint3 idx = SRV4[ti];

        float3 v0 = SRV2[idx.x].xyz;
        float3 v1 = SRV2[idx.y].xyz;
        float3 v2 = SRV2[idx.z].xyz;

        // project vertices to screen
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

        if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY)
            continue;

        // barycentric / edge functions (2D)
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
            float ndcZInterp = (w0 * (nz0)*iw0 + w1 * (nz1)*iw1 + w2 * (nz2)*iw2) / denom;

            // Use ndcZInterp (or convert to view-space depth if needed)
            // Here we treat a smaller value as closer if your NDC mapping is such; adjust comparison accordingly
            if (ndcZInterp < bestDepth)
            {
                bestDepth = ndcZInterp;

                // interpolate color with perspective correction:
                float4 c0 = SRV3[idx.x];
                float4 c1 = SRV3[idx.y];
                float4 c2 = SRV3[idx.z];

                float4 colorNumer = w0 * (c0 * iw0) + w1 * (c1 * iw1) + w2 * (c2 * iw2);
                float4 finalColor = colorNumer / denom;
                outColor = finalColor;
            }
        }
    }

    // Write final color to output buffer (flattened 2D -> 1D index)
    uint outIndex = py * (uint)ScreenSize.x + px;
    UAV0[outIndex] = outColor;// +SRV0[outIndex] / 1000.0;
}