/*
 * Temporal Anti-Alising with Compute Shader
 *
 *
 */

cbuffer TaaSettings : register(b0, space0) {
  float4 g_render_info;
};

uint get_frame_id() {
  return uint(g_render_info.x);
}

Texture2D<float4> g_history_input : register(t0, space0);
Texture2D<float4> g_frame_input : register(t1, space0);
RWTexture2D<float4> g_history_output : register(u0, space0);
RWTexture2D<float4> g_frame_output : register(u1, space0);
// TODO depth and velocity for reprojection

[numthreads(8, 8, 1)]
void CS(uint3 tid : SV_DispatchThreadID) {
  uint2 pos = tid.xy;
  float4 history = g_history_input[pos];
  float4 input = g_frame_input[pos];
  float4 output = lerp(history, input, 0.1);
  if (get_frame_id() == 0) { output = input; }
  g_history_output[pos] = output;
  g_frame_output[pos] = output;
}