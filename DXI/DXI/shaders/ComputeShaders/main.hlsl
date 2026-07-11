// Simple HLSL compute shader
// - 3 SRVs (Texture2D) at t0,t1,t2
// - 1 UAV (RWTexture2D) at u0
// - Constant buffer (b0) containing Projection and View matrices

cbuffer CameraCB : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjeMatrix;
    float4x4 ViewProjMatrix;
};

// input SRVs
Texture2D<float4> SRV0 : register(t0);
Texture2D<float4> SRV1 : register(t1);
// output UAV
RWTexture2D<float4> UAV0 : register(u0);

// thread group size (adjust to your dispatch)
[numthreads(16, 16, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    //// Query dimensions from one of the SRVs (assume all SRVs match)
    //uint width = 0;
    //uint height = 0;
    //g_SRV0.GetDimensions(width, height);

    //// Bounds check to avoid out-of-range Load
    //if (coord.x >= width || coord.y >= height)
    //{
    //    return;
    //}

    //// Read texels with Load (int3: x,y,mip)
    //float4 a = g_SRV0.Load(int3(coord, 0));
    //float4 b = g_SRV1.Load(int3(coord, 0));
    //float4 c = g_SRV2.Load(int3(coord, 0));

    //// Simple blend example
    //float4 result = (a + b + c) / 3.0f;

    //// Example usage of matrices (no-op here, shown for illustration):
    //// float4 pos = float4(coord.x, coord.y, 0.0f, 1.0f);
    //// pos = mul(pos, View);
    //// pos = mul(pos, Projection);
    //// (Use transformed pos if you map coords differently.)

    //g_Output[coord] = result;
}