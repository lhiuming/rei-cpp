#include "common.hlsl"
#include "hybrid_common.hlsl"
#include "halton.hlsl"
#include "procedural_env.hlsl"

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

struct BRDFSample {
  float3 wi;
  float3 specular;
  float3 diffuse;
  float importance;
};

BRDFSample sample_blinn_phong_brdf(BXDFSurface bxdf, float3 wo, float rnd0, float rnd1) {
  BRDFSample ret;
  // compute local random direction
  // NOTE: for a uniform distribution on hemisphere, phi = acos(rnd1)
  // ref: http://mathworld.wolfram.com/SpherePointPicking.html
  float theta = rnd0 * PI_2;
  float cos_phi = rnd1;
  float sin_phi = sqrt(1 - cos_phi * cos_phi);
  float3 dir = {sin_phi * cos(theta), cos_phi, sin_phi * sin(theta)};
  // we are sampling uniformly on hemisphere using sphere coordinate
  ret.importance = c_pi_2;

  // rotate to world space
  float3 normal = bxdf.normal;
  float3 pre_tangent = wo;
  if (abs(dot(normal, pre_tangent)) >= (1 - EPS) ) { pre_tangent = float3(wo.z, 0, -wo.x); }
  float3 bitangent = normalize(cross(pre_tangent, normal));
  float3 tangent = cross(normal, bitangent);
  ret.wi = dir.x * tangent + dir.y * normal + dir.z * bitangent;

  // evaluate brdf
  Space space;
  space.wi = ret.wi;
  space.wo = wo;
  BrdfCosine brdf_cosine;
  //brdf_cosine = blinn_phong_classic(bxdf, space);
  brdf_cosine = BRDF_blinn_phong_modified(bxdf, space);
  // note: the brdf result is alredy cosine-weighted
  ret.specular = brdf_cosine.specular;
  ret.diffuse = brdf_cosine.diffuse;

  return ret;
}

float3 integrate_blinn_phong(in float3 pos, in float3 wo, in Surface surf, float recur_depth) {
  HaltonState halton = halton_init(DispatchRaysIndex().xy, get_frame_id(g_render), c_frame_loop);

  BXDFSurface bxdf;
  bxdf.normal = surf.normal;
  bxdf.roughness_percepted = saturate(1 - surf.smoothness);
  bxdf.fresnel_0 = lerp(0.03, surf.color, surf.metalness);
  bxdf.albedo = lerp(surf.color, 0, surf.metalness);

  float3 accumulated = float3(0, 0, 0);

  const uint c_sample = min(4, c_halton_max_sample_2d);
  RayPayload payload = {{0, 0, 0, 0}, recur_depth + 1};
  RayDesc ray;
  ray.Origin = pos + surf.normal * 0.001; // to avoid self-intrusion
  ray.TMin = 0.0001;
  ray.TMax = 100.0;
  for (int i = 0; i < c_sample; i++) {
    float rnd0 = frac(halton_next(halton));
    float rnd1 = frac(halton_next(halton));
    BRDFSample samp = sample_blinn_phong_brdf(bxdf, wo, rnd0, rnd1);

    ray.Direction = samp.wi;
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, payload);
    // Convert color to irradiance
    float3 irradiance = payload.color.xyz;
    float3 radiance = (samp.specular + samp.diffuse) * irradiance;
    accumulated += samp.importance * radiance;
  }

  // Emissive is defined againat Lambertian response
  float3 emittance = surf.emissive * PI;
  float3 reflectance = accumulated / c_sample;
  return emittance + reflectance;
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
  if (depth <= c_epsilon) {
    float3 view_dir = -wo;
    output(sample_sky(view_dir));
    return;
  }

  Surface s;
  fill_surface(g_rt0[id], g_rt1[id], g_rt2[id], s);
  float3 radiance = integrate_blinn_phong(world_pos, wo, s, 0);

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

  // collect bounced irradiance
  float3 radiance;
  if (payload.depth < 2) {
    // Bounc around the scene !
    float3 world_pos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3 wo = -WorldRayDirection();

    // TODO read material from shader table
    Surface surf;
    fill_surface(g_material, n, surf);
    radiance = integrate_blinn_phong(world_pos, wo, surf, payload.depth);
  } else {
    // kill the path
    radiance = 0.001;
  }

  payload.color.xyz = radiance;
}

[shader("miss")] void miss_shader(inout RayPayload payload) {
  float3 dir = WorldRayDirection();
  payload.color = sample_sky(dir);
}
