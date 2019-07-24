#ifndef REI_DEFERRED_COMMON_HLSL
#define REI_DEFERRED_COMMON_HLSL

struct ConstBufferPerRender {
  float4 screen;
  float4x4 camera_world_trans;
  float4x4 camera_view_proj_inv;
  float4 camera_pos;
};

#endif
