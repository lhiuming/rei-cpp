/*
cbuffer cbPerFrame : register(b1) {
  Light light;
  float4x4 camera_world_trans;
  float3 camera_pos;
};
*/

// G-buffers
sampler samp : register(s0);
Texture2D<float> depth_buffer : register(t0);

float4 VS(uint vid : SV_VertexID) : SV_POSITION {
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

  if (vid == 0) return float4(-1, -3, 1, 1);
  if (vid == 1) return float4(+3, +1, 1, 1);
  if (vid == 2) return float4(-1, +1, 1, 1);

  return float4(0, 0, 0, 1);
}

float4 PS(float4 pos : SV_POSITION) : SV_TARGET {
  float2 uv = pos.xy / float2(1080, 720);
  float z = depth_buffer.Sample(samp, uv);
  return float4(z, z, z, 1);
}
