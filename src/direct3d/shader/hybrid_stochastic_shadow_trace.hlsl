#include "common.hlsl"
#include "hybrid_common.hlsl"

// Global //

// Output buffer
// NOTE: accumulating S_n
RWTexture2D<float4> g_shadowed_radiance: register(u0, space0);

// Scene TLAS
RaytracingAccelerationStructure g_scene : register(t0, space0);
// G-Buffer
Texture2D<float> g_depth : register(t1, space0);
// Stochastic samples
Texture2D<float4> g_stochastic_sample_ray: register(t2, space0);
Texture2D<float4> g_stochastic_sample_radiance: register(t3, space0);

// Per-render CB
ConstantBuffer<PerRenderConstBuffer> g_render : register(b0, space0);

// Raytracing Structs //

typedef BuiltInTriangleIntersectionAttributes HitAttr;

struct RayPayload {
  bool hit;
};

struct StochasticSample {
  float3 ray_dir;
  float ray_start;
  float ray_end;
  float3 radiance;
};

StochasticSample get_sample(uint2 id) {
  float4 ray = g_stochastic_sample_ray[id];
  float4 radiance = g_stochastic_sample_radiance[id];
  StochasticSample ret;
  ret.ray_dir = ray.xyz;
  ret.ray_start= ray.w;
  ret.radiance = radiance.xyz;
  ret.ray_end = radiance.w;
  return ret;
}

[shader("raygeneration")] void raygen_shader() {
  uint2 id = DispatchRaysIndex().xy;
  float depth = g_depth[id];

  float2 uv = float2(id) / float2(DispatchRaysDimensions().xy);
  float4 ndc = float4(uv * 2.f - 1.f, depth, 1.0f);
  ndc.y = -ndc.y;
  float4 world_pos_h = mul(g_render.proj_to_world, ndc);
  float3 world_pos = world_pos_h.xyz / world_pos_h.w;

  // reject on background
  if (depth <= EPS) {
    return;
  }

  // read sample
  StochasticSample ssample = get_sample(id);
  if (ssample.ray_end <= EPS) { return; }

  // Trace shadow ray
  RayPayload payload = {true};
  RayDesc ray;
  ray.Origin = world_pos;
  ray.Direction = ssample.ray_dir;
  ray.TMin = ssample.ray_start;
  ray.TMax = ssample.ray_end;
  TraceRay(g_scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH & RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, ~0,
    0, 0, 0, ray, payload);

  float3 radiance = payload.hit ? 0 : ssample.radiance;
  g_shadowed_radiance[id] += float4(radiance, 0.0);
}

[shader("closesthit")] void closest_hit_shader(inout RayPayload payload, in HitAttr attr) {}

[shader("miss")] void miss_shader(inout RayPayload payload) {
  payload.hit = false;
}
