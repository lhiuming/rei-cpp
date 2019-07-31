/*
 * Full-screen blit shader.
 */

#include "raster_common.hlsl"

sampler samp : register(s0, space0);
Texture2D<float4> input_texture : register(t0, space0);

struct Varying {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

Varying VS(uint vid : SV_VertexID) {
  Varying v;
  v.pos = make_screen_triangle_ndc(vid);
  v.uv = make_screen_triangle_uv(vid);
  return v;
}

float4 PS(Varying v) : SV_TARGET {
  // float2 screen_size = g_render.screen.xy;
  // float2 uv = pos.xy / screen_size;
  return input_texture.Sample(samp, v.uv);
}
