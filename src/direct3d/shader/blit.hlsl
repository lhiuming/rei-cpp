/*
 * Full-screen blit shader.
 */

sampler samp : register(s0, space0);
Texture2D<float4> input_texture : register(t0, space0);

struct Varying {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

Varying VS(uint vid : SV_VertexID) {
  /*
   *  2 --------------------- 1
   *  |     .   / |         /
   *  |     . /   |       /
   *  | - - * - - |     /
   *  |   / .     |   /
   *  | /   .     | /
   *  |-----------/
   *  |         /
   *  |       /
   *  |     /
   *  |   /
   *  | /
   *  0
   */

  Varying v;

  if (vid == 0) v.pos = float4(-1, -3, 1, 1);
  if (vid == 1) v.pos = float4(+3, +1, 1, 1);
  if (vid == 2) v.pos = float4(-1, +1, 1, 1);

  // ASSUME texcoord start from upper left
  if (vid == 0) v.uv = float2(0, 2);
  if (vid == 1) v.uv = float2(2, 0);
  if (vid == 2) v.uv = float2(0, 0);

  return v;
}

float4 PS(Varying v) : SV_TARGET {
  // float2 screen_size = g_render.screen.xy;
  // float2 uv = pos.xy / screen_size;
  return input_texture.Sample(samp, v.uv);
}
