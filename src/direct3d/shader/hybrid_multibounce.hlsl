#include "common.hlsl"
#include "hybrid_common.hlsl"
#include "halton.hlsl"
#include "integrator.hlsl"
#include "procedural_env.hlsl"

#define BOUNCE_LIMIT  2
#define FIRST_BOUNCE_SAMPLE 8
#define SECOND_BOUNCE_SAMPLE 2

struct Vertex {
  // NOTE: see VertexElement in cpp code
  float4 pos;
  float4 color;
  float3 normal;
};

// Global //

// Output buffer
RWTexture2D<float4> g_render_target : register(u0, space0);

// Scene TLAS
RaytracingAccelerationStructure g_scene : register(t0, space0);
// G-Buffer
Texture2D<float> g_depth : register(t1, space0);
Texture2D<float4> g_rt0: register(t2, space0);
Texture2D<float4> g_rt1: register(t3, space0);
Texture2D<float4> g_rt2: register(t4, space0);

// Per-render CB
ConstantBuffer<PerRenderConstBuffer> g_render : register(b0, space0);

// Local: Hit Group //

//  mesh data
ByteAddressBuffer g_indicies : register(t0, space1);
StructuredBuffer<Vertex> g_vertices : register(t1, space1);

// inplace material data
ConstantBuffer<PerMaterialConstBuffer> g_material : register(b0, space1);

// Raytracing Structs //

typedef BuiltInTriangleIntersectionAttributes HitAttr;

struct RayPayload {
  float4 color;
  int depth;
};

// REMARK: copy from dx-sample tutorial
uint3 Load3x16BitIndices(uint offsetBytes) {
  uint3 indices;

  // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
  // Since we need to read three 16 bit indices: { 0, 1, 2 }
  // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
  // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
  // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
  //  Aligned:     { 0 1 | 2 - }
  //  Not aligned: { - 0 | 1 2 }
  const uint dwordAlignedOffset = offsetBytes & ~3;
  const uint2 four16BitIndices = g_indicies.Load2(dwordAlignedOffset);

  // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
  if (dwordAlignedOffset == offsetBytes) {
    indices.x = four16BitIndices.x & 0xffff;
    indices.y = (four16BitIndices.x >> 16) & 0xffff;
    indices.z = four16BitIndices.y & 0xffff;
  } else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
  {
    indices.x = (four16BitIndices.x >> 16) & 0xffff;
    indices.y = four16BitIndices.y & 0xffff;
    indices.z = (four16BitIndices.y >> 16) & 0xffff;
  }

  return indices;
}

// NOTE: return a comforatble radiance
float4 sample_sky(float3 dir) {
  return float4(gradiant_sky_eva(dir), 1.0f);
}

float3 evaluate_emittion(Surface surf) {
  // Emissive is defined againat Lambertian response
  float3 emittance = surf.emissive * PI;
  return emittance;
}

float3 evaluate_lighting(float3 pos, float3 wo, Surface surf, float recur_depth, uint sample_count) {
  HaltonState halton = halton_init(DispatchRaysIndex().xy, get_frame_id(g_render), c_frame_loop);
  UniformHSampler h_sampler = uniform_h_sampler_init(halton, surf.normal, wo);

  BXDFSurface bxdf = to_bxdf(surf);

  float3 accumulated = float3(0, 0, 0);
  sample_count = min(sample_count, HALTON_MAX_SAMPLE_2D);

  RayPayload payload = {{0, 0, 0, 0}, recur_depth + 1};
  RayDesc ray;
  ray.Origin = pos;
  ray.TMin = EPS;
  ray.TMax = 100.0;
  for (int i = 0; i < sample_count; i++) {
    // Take sample
    SphereSample ssample = sampler_next(h_sampler);

    // Trace
    ray.Direction = ssample.wi;
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, payload);

    // Calculate radiance
    BrdfCosine brdf_cos = BRDF_GGX_Lambertian(bxdf, wo, ssample.wi);
    float3 irradiance = payload.color.xyz;
    float3 radiance = (brdf_cos.specular + brdf_cos.diffuse) * irradiance;
    
    // Sum
    accumulated += ssample.importance * radiance;
  }

  float3 reflectance = accumulated / sample_count;
  return reflectance;
}

void output(float4 color) {
  g_render_target[DispatchRaysIndex().xy] = color;
}

void output(float3 color) {
  output(float4(color, 1.0));
}

[shader("raygeneration")] void raygen_shader() {
  uint2 id = DispatchRaysIndex().xy;
  float depth = g_depth[id];

  float2 uv = float2(id) / float2(DispatchRaysDimensions().xy);
  float4 ndc = float4(uv * 2.f - 1.f, depth, 1.0f);
  ndc.y = -ndc.y;
  float4 world_pos_h = mul(g_render.proj_to_world, ndc);
  float3 world_pos = world_pos_h.xyz / world_pos_h.w;

  float3 wo = normalize(g_render.camera_pos.xyz - world_pos);

  // reject on background
  if (depth <= EPS) {
    float3 view_dir = -wo;
    output(sample_sky(view_dir));
    return;
  }

  Surface s;
  fill_surface(g_rt0[id], g_rt1[id], g_rt2[id], s);
  float3 radiance = evaluate_emittion(s) + evaluate_lighting(world_pos, wo, s, 0, FIRST_BOUNCE_SAMPLE);

  output(radiance);
}

[shader("closesthit")] void closest_hit_shader(inout RayPayload payload, in HitAttr attr) {
  // Get hitpoint normal from mesh data
  uint indexSizeInBytes = 2;
  uint indicesPerTriangle = 3;
  uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
  uint baseIndex = PrimitiveIndex() * triangleIndexStride;
  // load up 3 16 bit indices for the triangle.
  const uint3 indices = Load3x16BitIndices(baseIndex);
  // retrieve corresponding vertex normals for the triangle vertices.
  float3 n0 = g_vertices[indices[0]].normal.xyz;
  float3 n1 = g_vertices[indices[1]].normal.xyz;
  float3 n2 = g_vertices[indices[2]].normal.xyz;

  float2 bary = attr.barycentrics.xy;
  float3 n = n0 + bary.x * (n1 - n0) + bary.y * (n2 - n0);
  n = normalize(n);

  Surface surf;
  fill_surface(g_material, n, surf);
  float3 emittance = evaluate_emittion(surf);

  // collect bounced irradiance
  float3 reflectance;
  if (payload.depth < BOUNCE_LIMIT) {
    // Bounc around the scene !
    float3 pos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 wo = -WorldRayDirection();
    reflectance = evaluate_lighting(pos, wo, surf, payload.depth, SECOND_BOUNCE_SAMPLE);
  } else {
    // kill the path
    float3 ambient = 0;
    reflectance = ambient;
  }

  payload.color.xyz = emittance + reflectance;
}

[shader("miss")] void miss_shader(inout RayPayload payload) {
  float3 dir = WorldRayDirection();
  payload.color = sample_sky(dir);
}
