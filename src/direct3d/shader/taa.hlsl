/*
 * Temporal Anti-Alising with Compute Shader
 *
 *
 */

cbuffer TaaSettings : register(b0, space0) {
  float4 parset0;
};

uint get_frame_id() {
  return uint(parset0.x);
}

float get_blend_factor() {
  return parset0.y;
}

Texture2D<float4> g_history_input : register(t0, space0);
Texture2D<float4> g_frame_input : register(t1, space0);
RWTexture2D<float4> g_history_output : register(u0, space0);
RWTexture2D<float4> g_frame_output : register(u1, space0);
// TODO depth and velocity for reprojection

[numthreads(8, 8, 1)] void CS(uint3 tid
                              : SV_DispatchThreadID) {
  uint2 pos = tid.xy;
  float4 input = g_frame_input[pos];
  float4 output;
  if (get_frame_id() == 0) {
    output = input;
  } else {
    float4 history = g_history_input[pos];
    output = lerp(history, input, get_blend_factor());
  }
  g_history_output[pos] = output;
  g_frame_output[pos] = output;
}