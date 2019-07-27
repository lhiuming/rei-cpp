#ifndef REI_HYBRID_COMMON_HLSL
#define REI_HYBRID_COMMON_HLSL

struct PerRenderConstBuffer {
  // NOTE: see PerFrameConstantBuffer in cpp code
  float4 screen;
  float4x4 proj_to_world;
  float4 camera_pos;
};

struct PerMaterialConstBuffer {
  float4 albedo;
  float4 sme_unused;
};

float3 get_albedo(in PerMaterialConstBuffer mat) { return mat.albedo.xyz; }
float get_smoothness(in PerMaterialConstBuffer mat) { return mat.sme_unused.x; }
float get_metalness(in PerMaterialConstBuffer mat) { return mat.sme_unused.y; }
float get_emissive(in PerMaterialConstBuffer mat) { return mat.sme_unused.z; }

#endif