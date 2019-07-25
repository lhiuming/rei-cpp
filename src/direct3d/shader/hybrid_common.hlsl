#ifndef REI_HYBRID_COMMON_HLSL
#define REI_HYBRID_COMMON_HLSL

struct PerRenderConstBuffer {
  // NOTE: see PerFrameConstantBuffer in cpp code
  float4 screen;
  float4x4 proj_to_world;
  float4 camera_pos;
};

#endif