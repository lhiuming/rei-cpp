#include "common.hlsl"
#include "hybrid_common.hlsl"
#include "raster_common.hlsl"

// Output
RWTexture2D<float4> g_color: register(u0, space1);

// Per render info
ConstantBuffer<PerRenderConstBuffer> g_render : register(b0, space1);

void output(uint2 index, float3 radiance) {
  g_color[index] += float4(radiance, 1.0);
}

[numthreads(8, 8, 1)]
void CS(uint3 tid :SV_DispatchThreadId) {
  g_color[tid.xy] = 0;
}
